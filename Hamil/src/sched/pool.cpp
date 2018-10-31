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

  win32::Mutex mutex;
  win32::ConditionVariable cv;

  FreeListAllocator jobs_alloc;
  std::vector<IJob *> jobs;

  std::vector<WorkerPool::JobId> job_queue;

  std::atomic<bool> done;

protected:
  WorkerPoolData() :
    jobs_alloc(JobsPoolSize)
  {
    jobs.reserve(JobsPoolSize);
    job_queue.reserve(JobsQueueSize);
  }

private:
  friend WorkerPool;
};

WorkerPool::WorkerPool() :
  m_data(new WorkerPoolData())
{
  m_num_workers = win32::cpuinfo().numLogicalProcessors();
  for(size_t i = 0; i < m_num_workers; i++) {
    auto fn = std::bind(&WorkerPool::doWork, this);
    auto worker = new win32::Thread(fn);

    m_workers.push_back(worker);

    worker->dbg_SetName(util::fmt("WorkerPool_Worker%zu", i).data());
  }
}

WorkerPool::~WorkerPool()
{
  for(auto& worker : m_workers) {
    delete worker;
  }
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
  job->condition().sleep(mutex, [&]() { return job->done(); });

  m_data->jobs_alloc.dealloc(id, 1);
}

ulong WorkerPool::doWork()
{
  auto& mutex = m_data->mutex;
  auto& cv = m_data->cv;
  auto& done = m_data->done;

  auto& queue = m_data->job_queue;

  while(!done.load()) {
    mutex.acquire();
    cv.sleep(mutex, [&]() { return !queue.empty(); });

    // Grab a Job
    auto job_id = queue.back();

    // And remove it from the queue
    queue.pop_back();

    auto job = m_data->jobs.at(job_id);
    mutex.release();

    job->perform();
  }

  return 0;
}

}