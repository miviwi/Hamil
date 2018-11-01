#include <sched/pool.h>

#include <util/allocator.h>
#include <util/format.h>
#include <win32/cpuinfo.h>
#include <win32/thread.h>
#include <win32/mutex.h>
#include <win32/conditionvar.h>

#include <functional>
#include <atomic>

namespace sched {

class WorkerPoolData {
public:
  enum {
    JobsPoolSize = 1024,
    JobsQueueSize = 64,
  };

  // This mutex is shared by the workers waiting on the queue
  //   and thread(s) waiting for jobs to complete in waitJob()
  win32::Mutex mutex;

  // Workers sleep() on this while the 'job_queue' is empty
  win32::ConditionVariable cv;

  FreeListAllocator jobs_alloc;
  std::vector<IJob *> jobs;  // Job pool

  // - Scheduler does push_back()
  // - Workers do pop_back()
  // TODO: Replace std::vector with a lock-free queue
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

WorkerPool::JobId WorkerPool::scheduleJob(IJob *job)
{
  auto lock_guard = m_data->mutex.acquireScoped();

  // Add the Job to the pool
  auto& jobs = m_data->jobs;
  auto job_id = (JobId)m_data->jobs_alloc.alloc(1);
  if(job_id == jobs.size()) {
    jobs.push_back(job);
  } else {
    jobs[job_id] = job;
  }

  // Schedule the Job
  m_data->job_queue.push_back(job_id);

  // Wake up a worker to perform it
  m_data->cv.wake();

  return job_id;
}

void WorkerPool::waitJob(JobId id)
{
  auto& mutex = m_data->mutex;
  auto lock_guard = mutex.acquireScoped();

  auto job = m_data->jobs.at(id);
  assert(job && "Attempted to waitJob() on an invalid Job!");

  job->condition().sleep(mutex, [&]() { return job->done(); });

  // Remove the Job from the pool
  m_data->jobs_alloc.dealloc(id, 1);
  m_data->jobs.at(id) = nullptr;
}

WorkerPool& WorkerPool::kickWorkers()
{
  // Make sure the workers don't terminate immediately
  m_data->done.store(false);

  for(size_t i = 0; i < m_num_workers; i++) {
    auto fn = std::bind(&WorkerPool::doWork, this);
    auto worker = new win32::Thread(fn);

    m_workers.append(worker);

    worker->dbg_SetName(util::fmt("WorkerPool_Worker%zu", i).data());
  }

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

  while(!done.load()) { // done.load() == true indicates we should terminate
    mutex.acquire();

    // Sleep until there's something in the queue or killWorkers() was called
    cv.sleep(mutex, [&]() { return !queue.empty() || done.load(); });

    if(done.load()) { // Bail right away if killWorkers() was called
      mutex.release();
      return 0;
    }

    // Grab a Job
    auto job_id = queue.back();

    // ...and remove it from the queue
    queue.pop_back();

    auto job = m_data->jobs.at(job_id);
    mutex.release();

    assert(job && "Attempted to perform() invalid Job!");

    job->perform();
  }

  return 0;
}

}