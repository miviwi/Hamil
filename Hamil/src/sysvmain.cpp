#include <common.h>

#include <os/os.h>
#include <os/cpuid.h>
#include <os/time.h>
#include <os/thread.h>
#include <os/window.h>
#include <sysv/window.h>
#include <sysv/glcontext.h>
#include <sched/job.h>
#include <sched/pool.h>
#include <gx/gx.h>
#include <gx/info.h>
#include <util/unit.h>
#include <ft/font.h>

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

// OpenGL
#include <GL/gl3w.h>

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

#if 0
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
#endif

  sysv::Window window(1280, 720);
  sysv::GLContext gl_context;

  gl_context
    .acquire(&window)
    .makeCurrent();

  gx::init();
  ft::init();

  printf("extension(EXT::TextureSRGB):      %i\n", gx::info().extension(gx::EXT::TextureSRGB));
  printf("extension(ARB::ComputeShader):    %i\n", gx::info().extension(gx::ARB::ComputeShader));
  printf("extension(ARB::BindlessTexture):  %i\n", gx::info().extension(gx::ARB::BindlessTexture));
  printf("extension(ARB::TextureBPTC):      %i\n", gx::info().extension(gx::ARB::TextureBPTC));

  ft::Font face(ft::FontFamily("/usr/share/fonts/TTF/segoeui.ttf"), 35);

  glClearColor(0.5f, 0.8f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  window.swapBuffers();

  do_sleep();

  gl_context.release();

  os::finalize();

  return 0;
}
