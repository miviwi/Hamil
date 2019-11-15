#include <sched/pool.h>

#include <util/allocator.h>
#include <util/format.h>
#include <os/cpuinfo.h>
#include <os/thread.h>
#include <os/mutex.h>
#include <os/conditionvar.h>
#include <os/window.h>
#include <os/glcontext.h>
#include <gx/context.h>


#include <config>

#include <algorithm>
#include <memory>
#include <atomic>
#include <map>
#include <functional>

#include <cassert>

// Uncomment this line so workers DON'T get locked to specific cores
//   - Setting the affinity is supposed to remove latency caused by the
//     OS shuffling workers between cores, but I'm not sure if it actually
//     does (may even do more harm than good). Testing needed (!)
//#define NO_SET_AFFINITY

namespace sched {

class WorkerPoolData {
public:
  enum {
    JobsPoolSize = 1024,
    JobsQueueSize = 64,
  };

  // The Window from which the GLContexts will be acquired,
  //   must be stored here because GLContext::acquireContext()
  //   is called after creating the workers (in kickWorkers())
  os::Window *window = nullptr;
  std::map<os::Thread::Id, std::unique_ptr<gx::GLContext>> contexts;

  // This mutex is shared by the workers waiting on the queue
  //   and thread(s) waiting for jobs to complete in waitJob()
  os::Mutex::Ptr mutex;

  // Workers sleep() on this while the 'job_queue' is empty
  os::ConditionVariable::Ptr cv;

  FreeListAllocator jobs_alloc;
  std::vector<IJob *> jobs;  // Job pool

  // - Scheduler does push_back()
  // - Workers do pop_back()
  // TODO: Replace std::vector with a lock-free queue (?)
  std::vector<WorkerPool::JobId> job_queue;

  // Used to sleep() on 'workers_idle'
  os::Mutex::Ptr workers_idle_mutex;
  // Signaled when all workers finish (i.e. !workers_active && queue.empty())
  os::ConditionVariable::Ptr workers_idle;

  // The number of currently active worker threads
  std::atomic<uint> workers_active = 0;

  // Set to true when workers should terminate
  std::atomic<bool> done = false;

protected:
  WorkerPoolData() :
    jobs_alloc(JobsPoolSize)
  {
    mutex = os::Mutex::alloc();
    cv = os::ConditionVariable::alloc();

    workers_idle_mutex = os::Mutex::alloc();
    workers_idle = os::ConditionVariable::alloc();

    jobs.reserve(JobsPoolSize);
    job_queue.reserve(JobsQueueSize);

    done.store(false);
  }

private:
  friend WorkerPool;
};

WorkerPool::WorkerPool(int num_workers) :
  m_data(new WorkerPoolData())
{
  // Default number of workers == number of hardware threads
  m_num_workers = num_workers;
}

WorkerPool::~WorkerPool()
{
  killWorkers();
}

WorkerPool& WorkerPool::acquireWorkerGLContexts(os::Window& window)
{
  m_data->window = &window;

  return *this;
}

WorkerPool::JobId WorkerPool::scheduleJob(IJob *job)
{
  // Need to acquire the lock before modyfying the job pool
  auto lock_guard = m_data->mutex->acquireScoped();

  // Add the Job to the pool
  auto& jobs = m_data->jobs;
  auto id = (JobId)m_data->jobs_alloc.alloc(1);
  if(id == jobs.size()) {
    jobs.push_back(job);
  } else {
    assert(id != (JobId)FreeListAllocator::Error && "Too many jobs in pool!");
    assert(jobs.at(id) == nullptr && "Issued a JobId already in use!");

    jobs.at(id) = job;
  }

  // Schedule the Job
  m_data->job_queue.push_back(id);
  job->scheduled();

  // Wake up a worker to perform it
  m_data->cv->wake();

  return id;
}

void WorkerPool::waitJob(JobId id)
{
  assert(id != InvalidJob && "InvalidJob passed to waitJob()!");

  // Need to acquire the lock because we'll be
  //   removing the job from the pool
  auto& mutex = m_data->mutex;
  auto lock_guard = mutex->acquireScoped();

  auto& jobs = m_data->jobs;
  auto job = jobs.at(id);
  assert(job && "Attempted to waitJob() on an invalid Job!");

  // The Job was already waited on
  if(jobs.at(id) == nullptr) return;

  // Use a timeout here to prevent deadlocks, they can
  //   happen due to a race condition in IJob::finished()
  //   where the waiting thread goes to sleep just BEFORE
  //   the worker thread wakes condition()
  // TODO: figure out how to fix this, not bypass it...
  job->condition()->sleep(mutex, [&]() { return job->done(); }, 1);

  // Remove the Job from the pool and make sure it's 
  //   never slept on again until it's rescheduled
  m_data->jobs_alloc.dealloc(id, 1);
  jobs.at(id) = nullptr;
}

WorkerPool::JobId WorkerPool::jobId(IJob *job) const
{
  // Need to acquire the lock because we'll be
  //   querying the Job pool
  auto& mutex = m_data->mutex;
  auto lock_guard = mutex->acquireScoped();

  auto& jobs = m_data->jobs;
  auto it = std::find(jobs.cbegin(), jobs.cend(), job);

  if(it == jobs.cend()) return InvalidJob;

  // The index of the Job in the pool == Id
  return std::distance(jobs.cbegin(), it);
}

WorkerPool& WorkerPool::kickWorkers(const char *name)
{
  // Make sure the workers don't terminate immediately
  m_data->done.store(false);

  uint num_cores = os::cpuinfo().numLogicalProcessors();
  bool hyperthreading = os::cpuinfo().hyperthreading();

  if(!name) name = "WorkerPool_Worker";

  bool set_affinity = m_num_workers > 0;
  uint num_workers = m_num_workers > 0 ? (uint)m_num_workers : num_cores;

  for(size_t i = 0; i < num_workers; i++) {
    // ulong (WorkerPool::*fn)() -> ulong (*fn)()
    auto fn = std::bind(&WorkerPool::doWork, this);
    auto worker = os::Thread::alloc();

    worker->create(fn, /* suspended = */ true);

    m_workers.append(worker);

    // Give each worker a nice name
    worker->dbg_SetName(util::fmt("%s%zu", name, i).data());

#if !defined(NO_SET_AFFINITY)
    // Lock each worker to a given thread when the
    //  number of workers == number of HW threads
    //   - When the system has hyperthreading (SMT)
    //     sequential processor numbers signify
    //     hyperthreads, in that case group the
    //     workers into 2 sets
    uintptr_t thread = i;
    if(hyperthreading && set_affinity) {
      // The hyperthreads will be the 2nd group
      uintptr_t thread_group = i >= num_cores;

      // This works out to (for 2-cores/4-threads):
      //   i==0  =>  0     (group 1)
      //   i==1  =>  2     (group 1)
      //   i==2  =>  1     (group 2)
      //   i==3  =>  3     (group 2)
      thread = (i % num_cores)*2ull | thread_group;
    }

    worker->affinity(1ull << thread);
#endif
  }

  // Optionally acquire GLContexts for all the workers
  auto window = m_data->window;
  auto& contexts = m_data->contexts;
  if(window) {
    auto& current_context = gx::GLContext::current();

    for(auto& worker : m_workers) {
      auto gl_context = os::create_glcontext();
      gl_context->acquire(window, &current_context);

      contexts.emplace(worker->id(), std::move(gl_context));
    }
  }

  // Kick off the workers
  for(auto& worker : m_workers) worker->resume();

  return *this;
}

WorkerPool& WorkerPool::killWorkers()
{
  m_data->done.store(true);
  m_data->cv->wakeAll();

  for(auto& worker : m_workers) {
    // Wait for each worker before we delete it
    if(worker->exitCode() == os::Thread::StillActive) {
      worker->wait();
    }

    delete worker;
  }
  m_workers.clear();

  return *this;
}

WorkerPool& WorkerPool::waitWorkersIdle()
{
  auto& idle_mutex = m_data->workers_idle_mutex;
  auto lock_guard = idle_mutex->acquireScoped();

  auto& queue = m_data->job_queue;
  auto& workers_active = m_data->workers_active;

  m_data->workers_idle->sleep(idle_mutex, [&]() {
    // Avoid a situation where a job gets scheduled just
    //   as we've been woken up causing a spurious wakeup
    return queue.empty() && workers_active.load() < 1;
  });

  return *this;
}

ulong WorkerPool::doWork()
{
  auto& mutex = *m_data->mutex;
  auto& cv = *m_data->cv;
  auto& done = m_data->done;

  auto& queue = m_data->job_queue;

  auto& workers_active = m_data->workers_active;

  // If GLContexts were acquired for the workers - grab one
  auto& contexts = m_data->contexts;
  auto context_it = contexts.find(os::Thread::current_thread_id());

  gx::GLContext *context = nullptr;
  if(context_it != contexts.end()) {
    context = context_it->second.get();
    context->makeCurrent();
  }

  while(!done.load()) { // done.load() == true indicates we should terminate
    mutex.acquire();  // We need to lock to sleep and to access the Job pool/queue

    // Sleep until there's something in the queue or killWorkers() was called
    cv.sleep(mutex, [&]() { return !queue.empty() || done.load(); });

    if(done.load()) {
      mutex.release();
      break; // Bail right away if killWorkers() was called
    }

    auto job_id = queue.back(); // Grab a Job
    queue.pop_back();           // ...and remove it from the queue

    auto job = m_data->jobs.at(job_id);
    mutex.release(); // We're done accessing the Job pool/queue
                     //   so the lock can be safely released here

    assert(job && "Attempted to perform() an invalid Job!");

    workers_active++;  // Work started...
    job->perform();

    // If the previous value of workers_active is 1
    //   that means we were the only active worker
    bool workers_idle = workers_active.fetch_sub(1) < 2;

    if(workers_idle && queue.empty()) m_data->workers_idle->wakeAll();
  }

  if(context) context->release();

  return 0;
}

}
