#include "os/input.h"
#include <common.h>

#include <os/os.h>
#include <os/cpuid.h>
#include <os/time.h>
#include <os/thread.h>
#include <os/window.h>
#include <cli/cli.h>
#include <sysv/window.h>
#include <sysv/glcontext.h>
#include <sched/job.h>
#include <sched/pool.h>
#include <gx/gx.h>
#include <gx/info.h>
#include <gx/pipeline.h>
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

int main(int argc, char *argv[])
{
  os::init();

  if(argc > 1) {
    if(auto exit_code = cli::args(argc, argv)) exit(exit_code);
  }

  sysv::Window window(1280, 720);

  window.initInput();

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

  ft::Font face(ft::FontFamily("/usr/share/fonts/TTF/DejaVuSerif.ttf"), 20);

  glClearColor(0.5f, 0.8f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  window.swapBuffers();

  while(window.processMessages()) {
    os::Timers::tick();

    while(auto input = window.getInput()) {
#if 1
      if(auto mouse = input->get<os::Mouse>()) {
        auto btn_state_str = [&](os::Mouse::Button btn) -> const char * {
          return mouse->buttons & btn ? "down" : "up";
        };

        if(mouse->event != os::Mouse::Move || true) {
          printf("got some input -> os::Mouse(%s) [dx,dy]=[%.1f,%.1f] Left?=%s Right?=%s\n",
              mouse->dbg_TypeStr(),
              mouse->dx, mouse->dy,
              btn_state_str(os::Mouse::Left), btn_state_str(os::Mouse::Right)
          );
        }
      } else if(auto kb = input->get<os::Keyboard>()) {
        printf(
            "got some input -> os::Keyboard(%s)\n"
            "sym=%c time_held=%.2lf\n",
            kb->dbg_TypeStr(),
            (char)kb->sym, os::Timers::ticks_to_sf(kb->time_held));
      }
#endif

      if(auto kb = input->get<os::Keyboard>()) {
        if(kb->keyDown('Q')) window.quit();
      }
    }

    glClear(GL_COLOR_BUFFER_BIT);

    gx::Pipeline pipeline;
    pipeline
      .viewport(0, 0, 1280, 720)
      .alphaBlend()
      .use();


    face.draw("Hello, world!", { 100.0f, 200.0f }, vec3(1.0f));

    window.swapBuffers();    // Wait for v-sync
  }

  gl_context.release();

  os::finalize();

  return 0;
}
