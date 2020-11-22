#include "glcorearb.h"
#include "gx/framebuffer.h"
#include "res/handle.h"
#include "res/image.h"
#include "res/res.h"
#include "res/texture.h"
#include "ui/frame.h"
#include <GL/gl.h>
#include <common.h>

#include <resources.h>

#include <type_traits>
#include <util/polystorage.h>
#include <os/os.h>
#include <os/cpuid.h>
#include <os/path.h>
#include <os/time.h>
#include <os/thread.h>
#include <os/window.h>
#include <os/input.h>
#include <os/file.h>
#include <sysv/x11.h>
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
#include <ui/console.h>
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

int main(int argc, char *argv[])
{
  os::init();

  if(argc > 1) {
    if(auto exit_code = cli::args(argc, argv)) exit(exit_code);
  }

#if 1
  sysv::Window window(1280, 720);

  window.initInput();

  sysv::GLContext gl_context;

  gl_context
    .acquire(&window)
    .makeCurrent()
    .dbg_EnableMessages();

  gx::init();
  ft::init();
  res::init();
  ui::init();
  ek::init();

  res::load(R.image.res.images.ids);

  res::Handle<res::Image> r_pineapple = R.image.res.images.pineapple;

  printf("extension(EXT::TextureSRGB):      %i\n", gx::info().extension(gx::EXT::TextureSRGB));
  printf("extension(ARB::ComputeShader):    %i\n", gx::info().extension(gx::ARB::ComputeShader));
  printf("extension(ARB::BindlessTexture):  %i\n", gx::info().extension(gx::ARB::BindlessTexture));
  printf("extension(ARB::TextureBPTC):      %i\n", gx::info().extension(gx::ARB::TextureBPTC));

  gx::ResourcePool pool(1024);
  gx::MemoryPool memory(4096);

  ui::CursorDriver cursor(1280/2, 720/2);
  ui::Ui iface(pool, ui::Geometry(vec2(), vec2(1280.0f, 720.0f)), ui::Style::basic_style());

  auto pineapple = iface.drawable().fromImage(
      r_pineapple->data<u8>(),
      r_pineapple->width(), r_pineapple->height()
  );

  iface
    .frame(ui::create<ui::WindowFrame>(iface)
        .title("Window")
        .content(ui::create<ui::LabelFrame>(iface)
            .caption("Hello!")
            .color(ui::white()))
        .background(ui::blue().darkenf(0.8))
        .geometry(ui::Geometry(vec2(200.0f, 100.0f), vec2(150.0f, 150.0f)))
        .gravity(ui::Frame::Center))
    .frame(ui::create<ui::WindowFrame>(iface)
        .title("Pineapple")
        .content(ui::create<ui::LabelFrame>(iface)
          .drawable(pineapple))
        .background(ui::green().lightenf(0.8))
        .geometry(ui::Geometry(vec2(500.0f, 100.0f), vec2(512.0f, 512.0f))))
    .frame(ui::create<ui::ConsoleFrame>(iface, "g_console"))
    ;

  auto& ui_console = *iface.getFrameByName<ui::ConsoleFrame>("g_console");

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
#endif

      cursor.input(input);
      if(auto kb = input->get<os::Keyboard>()) {
        if(kb->keyDown('`')) {
          ui_console.toggle();
        } else if(kb->modifier(os::Keyboard::Alt) && kb->keyDown('Q')) {
          window.quit();
        }

#if 0
          printf("modifiers=%.4x key=(%x, %d, %c)\n"
              "     modifier(Keyboard::Alt)? %s\n",
              kb->modifiers,
              kb->key, kb->key, kb->key,
              kb->modifier(os::Keyboard::Alt) ? "yes" : "no");
#endif
      }

      if(iface.input(cursor, input)) continue;    // The input has already been handled by the ui


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
  window.destroy();
  
  sysv::x11_detail::x11().disconnect();
#endif
  os::finalize();

  return 0;
}