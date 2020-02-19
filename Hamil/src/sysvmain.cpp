#include "ui/layout.h"
#include "ui/cursor.h"
#include "glcorearb.h"
#include "gx/framebuffer.h"
#include "ui/frame.h"
#include <GL/gl.h>
#include <common.h>

#include <os/os.h>
#include <os/cpuid.h>
#include <os/time.h>
#include <os/thread.h>
#include <os/window.h>
#include <os/input.h>
#include <cli/cli.h>
#include <sysv/window.h>
#include <sysv/glcontext.h>
#include <sched/job.h>
#include <sched/pool.h>
#include <gx/gx.h>
#include <gx/info.h>
#include <gx/pipeline.h>
#include <gx/resourcepool.h>
#include <gx/commandbuffer.h>
#include <util/unit.h>
#include <ft/font.h>
#include <ui/ui.h>
#include <ui/uicommon.h>
#include <ui/style.h>
#include <ui/cursor.h>
#include <ui/layout.h>
#include <ui/window.h>
#include <ui/label.h>
#include <ek/euklid.h>
#include <ek/renderer.h>

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

  sysv::Window window(1282, 722);

  window.initInput();

  sysv::GLContext gl_context;

  gl_context
    .acquire(&window)
    .makeCurrent();

  gx::init();
  ft::init();
  ui::init();
//  ek::init();

  printf("extension(EXT::TextureSRGB):      %i\n", gx::info().extension(gx::EXT::TextureSRGB));
  printf("extension(ARB::ComputeShader):    %i\n", gx::info().extension(gx::ARB::ComputeShader));
  printf("extension(ARB::BindlessTexture):  %i\n", gx::info().extension(gx::ARB::BindlessTexture));
  printf("extension(ARB::TextureBPTC):      %i\n", gx::info().extension(gx::ARB::TextureBPTC));

  gx::ResourcePool pool(1024);
  gx::MemoryPool memory(4096);

  ui::CursorDriver cursor(1280/2, 720/2);
  ui::Ui iface(pool, ui::Geometry(vec2(), vec2(1280.0f, 720.0f)), ui::Style::basic_style());

  iface
    .frame(ui::create<ui::WindowFrame>(iface)
        .title("Window")
        .content(ui::create<ui::LabelFrame>(iface)
            .caption("Hello!")
            .color(ui::white()))
        .background(ui::blue().darken(0.8))
        .geometry(ui::Geometry(vec2(200.0f, 100.0f), vec2(150.0f, 150.0f)))
        .gravity(ui::Frame::Center))
    ;

  ft::Font face(ft::FontFamily("dejavu-serif"), 20);

  auto pipeline = gx::Pipeline()
      .viewport(0, 0, 1280, 720)
      .clearColor(vec4(0.2f, 0.4f, 0.2, 1.0f))
      .alphaBlend();

  window.swapBuffers();

  while(window.processMessages()) {
    os::Timers::tick();

    while(auto input = window.getInput()) {
#if 0
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

      cursor.input(input);
      if(iface.input(cursor, input)) continue;    // The input has already been handled by the ui

      if(auto kb = input->get<os::Keyboard>()) {
        if(kb->keyDown('Q')) window.quit();
      }
    }


    iface.paint()
       .execute();

    cursor.paint();

    pipeline.use();
    gx::Framebuffer::bind_window(gx::Framebuffer::Draw);
    glClear(GL_COLOR_BUFFER_BIT);

    auto& ui_fb = pool.get<gx::Framebuffer>(iface.framebufferId());

    ui_fb.blitToWindow(
        ivec4{ 0, 0, 1280, 720 },
        ivec4{ 0, 0, 1280, 720 },
        gx::Framebuffer::ColorBit,
        gx::Sampler::Nearest
    );

    window.swapBuffers();    // Wait for v-sync
  }

  gl_context.release();

  os::finalize();

  return 0;
}
