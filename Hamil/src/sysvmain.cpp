#include <common.h>

#include <os/os.h>
#include <os/cpuid.h>
#include <os/time.h>
#include <os/thread.h>
#include <sched/job.h>
#include <sched/pool.h>
#include <util/unit.h>

#include <numeric>
#include <vector>
#include <array>
#include <random>

#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cstdio>

// SysV
#include <unistd.h>
#include <time.h>

size_t do_sleep()
{
  std::random_device rd;
  std::uniform_int_distribution<useconds_t> distribution(2000, 5000);

  auto sleep_time_ms = distribution(rd);
  auto sleep_time_us = sleep_time_ms * 1000;

  printf("usleep(%zu /* %zums */)\n", (size_t)sleep_time_us, (size_t)sleep_time_ms);
  usleep(sleep_time_us);

  return sleep_time_ms;
}

int thread_proc()
{
  do_sleep();
  puts("    done!");

  return 1;
}

int main(void)
{
  os::init();

  sched::WorkerPool worker_pool;
  worker_pool.kickWorkers();

  auto a_job = sched::create_job([](int job_no) -> Unit {
    printf("job_no %d:\n", job_no);
    do_sleep();
    printf("    job_no %d done!\n", job_no);

    return {};
  });

  auto a_job1 = a_job.clone();
  auto a_job2 = a_job.clone();
  auto a_job3 = a_job.clone();
  
  worker_pool.scheduleJob(a_job1.withParams(1));
  worker_pool.scheduleJob(a_job2.withParams(2));
  worker_pool.scheduleJob(a_job3.withParams(3));

  worker_pool.waitWorkersIdle();

  worker_pool.killWorkers();

  os::finalize();

  return 0;
}
