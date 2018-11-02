#pragma once

#include <sched/scheduler.h>
#include <sched/job.h>

#include <util/smallvector.h>

#include <vector>
#include <memory>

namespace win32 {
class Thread;

class Window;
}

namespace sched {

// PIMPL class
class WorkerPoolData;

// Simple FIFO scheduler for asynchronous Job execution,
//  simple usage:
//     WorkerPool pool;
//     auto some_job = create_job([&](int x) -> int {
//       // This code will be run when the Job is scheduled
//       Sleep(1000);
//
//       // After waitJob() (so some_job.done() == true)
//       //    some_job.result() == x
//       return 42;
//     }, 0 /* default value for 'x' */);
//
//     // Job::withParams(...) saves parameters which will be passed
//     //   to the Jobs function and returns a pointe to the Job
//     auto some_job_id = pool.scheduleJob(someJob.withParams(42)); // x == 42
//
//     // No scheduled Jobs can run before this point
//     pool.kickWorkers();
//
//     /* ...do some work... */
//
//     pool.waitJob(some_job_id);
//     printf("result: %d\n", some_job.result());  // Prints "result: 42"
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

  // Must be called BEFORE kickWorkers()
  WorkerPool& acquireWorkerGlContexts(win32::Window& window);

  // The caller is responsible for freeing the Job
  //   - Thanks to this Jobs can be stack-allocated
  // A single Job instance can be scheduled multiple
  //   times, then the calling code must make sure
  //   one instance of a Job is NEVER scheduled 
  //   concurrently (i.e. only one pointer to a given
  //   Job can exist in the queue at any given time)
  JobId scheduleJob(IJob *job);

  // After this call the Job::result() can be obtained
  //   from the scheduled Job and job.done() == true
  //  - Each job MUST be waited on so the WorkerPool can
  //    remove it from it's queue
  //  - Waiting on a Job twice is a no-op
  void waitJob(JobId id);

  // O(n) complexity with respect to number of in-flight jobs
  JobId jobId(IJob *job) const;

  // Create and start the worker Threads
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