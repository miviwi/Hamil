#include <sched/pool.h>

#include <util/allocator.h>
#include <util/format.h>
#include <win32/cpuinfo.h>
#include <win32/thread.h>
#include <win32/mutex.h>
#include <win32/conditionvar.h>
#include <win32/window.h>
#include <win32/glcontext.h>

#include <cassert>
#include <map>
#include <functional>
#include <atomic>

namespace sched {

class WorkerPoolData {
public:
  enum {
    JobsPoolSize = 1024,
    JobsQueueSize = 64,
  };

  // The Window from which the OGLContexts will be acquired,
  //   must be stored here because OGLContext::acquireContext()
  //   is called after creating the workers (in kickWorkers())
  win32::Window *window;
  std::map<win32::Thread::Id, win32::OGLContext> contexts;

  // This mutex is shared by the workers waiting on the queue
  //   and thread(s) waiting for jobs to complete in waitJob()
  win32::Mutex mutex;

  // Workers sleep() on this while the 'job_queue' is empty
  win32::ConditionVariable cv;

  FreeListAllocator jobs_alloc;
  std::vector<IJob *> jobs;  // Job pool

  // - Scheduler does push_back()
  // - Workers do pop_back()
  // TODO: Replace std::vector with a lock-free queue (?)
  std::vector<WorkerPool::JobId> job_queue;

  // Set to true when workers should terminate
  std::atomic<bool> done;

protected:
  WorkerPoolData() :
    jobs_alloc(JobsPoolSize)
  {
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
  m_num_workers = num_workers > 0 ? (uint)num_workers : win32::cpuinfo().numLogicalProcessors();
}

WorkerPool::~WorkerPool()
{
  killWorkers();
}

WorkerPool& WorkerPool::acquireWorkerOGLContexts(win32::Window& window)
{
  m_data->window = &window;

  return *this;
}

WorkerPool::JobId WorkerPool::scheduleJob(IJob *job)
{
  // Need to acquire the lock before modyfying the job pool
  auto lock_guard = m_data->mutex.acquireScoped();

  // Add the Job to the pool
  auto& jobs = m_data->jobs;
  auto id = (JobId)m_data->jobs_alloc.alloc(1);
  if(id == jobs.size()) {
    jobs.push_back(job);
  } else {
    assert(jobs.at(id) == nullptr && "Issued a JobId already in use!");

    jobs[id] = job;
  }

  // Schedule the Job
  m_data->job_queue.push_back(id);
  job->scheduled();

  // Wake up a worker to perform it
  m_data->cv.wake();

  return id;
}

void WorkerPool::waitJob(JobId id)
{
  assert(id != InvalidJob && "InvalidJob passed to waitJob()!");

  // Need to acquire the lock because we'll be
  //   removing the job from the pool
  auto& mutex = m_data->mutex;
  auto lock_guard = mutex.acquireScoped();

  auto job = m_data->jobs.at(id);
  assert(job && "Attempted to waitJob() on an invalid Job!");

  // The Job was already waited on
  if(job->done()) return;

  job->condition().sleep(mutex, [&]() { return job->done(); });

  // Remove the Job from the pool and make sure it's 
  //   never waited on again until it's rescheduled
  m_data->jobs_alloc.dealloc(id, 1);
  m_data->jobs.at(id) = nullptr;
}

WorkerPool& WorkerPool::kickWorkers()
{
  // Make sure the workers don't terminate immediately
  m_data->done.store(false);

  for(size_t i = 0; i < m_num_workers; i++) {
    // ulong (WorkerPool::*fn)() -> ulong (*fn)()
    auto fn = std::bind(&WorkerPool::doWork, this);
    auto worker = new win32::Thread(fn, /* suspended = */ true);

    m_workers.append(worker);

    // Give each worker a nice name
    worker->dbg_SetName(util::fmt("WorkerPool_Worker%zu", i).data());
  }

  // Optionally acquire OGLContexts for all the workers
  auto window = m_data->window;
  auto& contexts = m_data->contexts;
  if(window) {
    for(auto& worker : m_workers) {
      contexts.emplace(worker->id(), window->acquireOGLContext());
    }
  }

  // Kick off the workers
  for(auto& worker : m_workers) worker->resume();

  return *this;
}

WorkerPool& WorkerPool::killWorkers()
{
  m_data->done.store(true);
  m_data->cv.wakeAll();

  for(auto& worker : m_workers) {
    // Wait for each worker before we delete it
    if(worker->exitCode() == win32::Thread::StillActive) {
      worker->wait();
    }

    delete worker;
  }
  m_workers.clear();

  return *this;
}

ulong WorkerPool::doWork()
{
  auto& mutex = m_data->mutex;
  auto& cv = m_data->cv;
  auto& done = m_data->done;

  auto& queue = m_data->job_queue;

  // If OGLContexts were acquired for the workers - grab one
  auto& contexts = m_data->contexts;
  auto context_it = contexts.find(win32::Thread::current_thread_id());

  win32::OGLContext *context = nullptr;
  if(context_it != contexts.end()) {
    context = &context_it->second;
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

    job->perform();
  }

  if(context) context->release();

  return 0;
}

}