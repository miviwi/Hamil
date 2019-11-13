#include <common.h>

#include <os/os.h>
#include <os/cpuid.h>
#include <os/time.h>
#include <os/thread.h>

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

  auto a_thread = os::Thread::alloc();

  puts("creating 'a_thread'...");
  a_thread->create(thread_proc, true);

  printf("a_thread.id()=%u\n", a_thread->id());

  a_thread->dbg_SetName("a_thread");

  os::Timers::tick();

  sleep(2);

  getchar();

  a_thread->resume();
  puts("resumed 'a_thread'...");

  auto wait_result = a_thread->wait(5000);
  printf("a_thread.wait()=0x%.8lX\n", wait_result);

  printf("'a_thread' exited with exitCode=%u\n", a_thread->exitCode());

  os::finalize();

  return 0;
}
