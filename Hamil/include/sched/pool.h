#pragma once

#include <sched/scheduler.h>
#include <sched/job.h>

#include <util/smallvector.h>

#include <vector>
#include <memory>

namespace win32 {
class Thread;
}

namespace sched {

// PIMPL class
class WorkerPoolData;

class WorkerPool {
public:
  using JobId = u32;

  enum : JobId {
    InvalidJob = ~0u,
  };

  // When not specified the number of workers defaults
  //   to the number of available hardware threads
  WorkerPool(int num_workers = -1);

  WorkerPool(const WorkerPool& other) = delete;
  ~WorkerPool();

  // The caller is responsible for freeing the Job
  //   - Thanks to this Jobs can be stack-allocated
  JobId scheduleJob(IJob *job);

  // After this call the result can be obtained from
  //   the scheduled Job
  // Each job MUST be waited on so the WorkerPool can
  //   remove it from it's queue
  void waitJob(JobId id);

  // Start the worker Threads
  WorkerPool& kickWorkers();

  // Stop the worker Threads
  WorkerPool& killWorkers();

private:
  // 16 inline threads ought to be enough to avoid heap allocation for most cases
  using WorkerVector = util::SmallVector<win32::Thread *, 16*sizeof(win32::Thread *)>;

  // Used as the Fn for worker win32::Thread()
  ulong doWork();

  WorkerPoolData *m_data;

  uint m_num_workers;
  WorkerVector m_workers;
};

}