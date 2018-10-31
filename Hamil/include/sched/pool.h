#pragma once

#include <sched/scheduler.h>
#include <sched/job.h>

#include <vector>
#include <memory>

namespace win32 {
class Thread;
}

namespace sched {

class WorkerPoolData;

class WorkerPool {
public:
  using JobId = u32;

  WorkerPool();
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

  void killWorkers();

private:
  // Used as the Fn for worker win32::Thread()
  ulong doWork();

  WorkerPoolData *m_data;

  uint m_num_workers;
  std::vector<win32::Thread *> m_workers;
};

}