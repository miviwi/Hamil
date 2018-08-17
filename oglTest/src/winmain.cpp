#include <util/format.h>
#include <util/opts.h>

#include <math/geometry.h>
#include <math/transform.h>

#include <win32/win32.h>
#include <win32/panic.h>
#include <win32/window.h>
#include <win32/time.h>
#include <win32/file.h>
#include <win32/stdstream.h>
#include <win32/mman.h>

#include <uniforms.h>
#include <gx/gx.h>
#include <gx/buffer.h>
#include <gx/vertex.h>
#include <gx/program.h>
#include <gx/pipeline.h>
#include <gx/texture.h>
#include <gx/framebuffer.h>

#include <ft/font.h>

#include <ui/ui.h>
#include <ui/cursor.h>
#include <ui/frame.h>
#include <ui/layout.h>
#include <ui/button.h>
#include <ui/slider.h>
#include <ui/label.h>
#include <ui/dropdown.h>
#include <ui/textbox.h>
#include <ui/console.h>

#include <py/python.h>
#include <py/object.h>
#include <py/exception.h>
#include <py/types.h>
#include <py/collections.h>

#include <yaml/document.h>
#include <yaml/node.h>

#include <res/res.h>
#include <res/manager.h>
#include <res/resource.h>
#include <res/text.h>
#include <res/shader.h>
#include <res/image.h>

#include <game/game.h>

#include <cli/cli.h>

#include <vector>
#include <array>
#include <utility>

//int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
int main(int argc, char *argv[])
{
  if(argc > 1) {
    if(auto exit_code = cli::args(argc, argv)) exit(exit_code);
  }

  win32::init();

  constexpr vec2 WindowSize = { 1280, 720 };

  using win32::Window;
  Window window(WindowSize.x, WindowSize.y);

  constexpr ivec2 FB_DIMS{ 1280, 720 };

  gx::init();
  ft::init();
  ui::init();
  py::init();
  res::init();
  game::init();

  ft::Font face(ft::FontFamily("georgia"), 35);
  ft::Font small_face(ft::FontFamily("segoeui"), 12);

  ui::CursorDriver cursor(1280/2, 720/2);
  vec3 pos{ 0, 0, 0 };
  float pitch = 0, yaw = 0;
  float zoom = 1.0f, rot = 0.0f;

  vec3 sun{ -120.0f, 160.0f, 140.0f };

  bool constrained = true;

  int animate = -1;

  auto fmt = gx::VertexFormat()
    .attr(gx::f32, 3)
    .attr(gx::f32, 3);

  auto r_texture = res::resource().load(
    res::resource().guid<res::Image>("tex", "")
  ).lock()->as<res::Image>();

  gx::Texture2D tex(gx::rgb);
  auto sampler = gx::Sampler::repeat2d_linear();

  tex.init(r_texture->data(), 0, r_texture->dimensions().x, r_texture->dimensions().y, gx::rgba, gx::u8);
  tex.generateMipmaps();

  auto r_phong = res::resource().load(
    res::resource().guid<res::Shader>("phong", "/shaders")
  ).lock()->as<res::Shader>();

  auto r_program = res::resource().load(
    res::resource().guid<res::Shader>("program", "/shaders")
  ).lock()->as<res::Shader>();

  auto r_tex = res::resource().load(
    res::resource().guid<res::Shader>("tex", "/shaders")
  ).lock()->as<res::Shader>();

  auto program     = gx::make_program(
    r_program->source(res::Shader::Vertex), r_program->source(res::Shader::Fragment), U.program);
  auto tex_program = gx::make_program(
    r_tex->source(res::Shader::Vertex), r_tex->source(res::Shader::Fragment), U.tex);

  program.label("program");
  tex_program.label("TEX_program");

  gx::Texture2D fb_tex(gx::rgb8);
  gx::Framebuffer fb;

  fb_tex.initMultisample(2, FB_DIMS.x, FB_DIMS.y);
  //fb_tex.init(FB_DIMS.x, FB_DIMS.y);
  fb_tex.label("FB_tex");

  fb.use()
    .tex(fb_tex, 0, gx::Framebuffer::Color(0))
    .renderbuffer(gx::depth24, gx::Framebuffer::Depth);
  if(fb.status() != gx::Framebuffer::Complete) {
    win32::panic("couldn't create main Framebuffer!", win32::FramebufferError);
  }

  fb.label("FB");

  gx::Texture2D shadowmap(gx::depthf);
  gx::Framebuffer fb_shadow;

  shadowmap.init(1024, 1024);
  shadowmap.label("shadowmap");

  fb_shadow.use()
    .tex(shadowmap, 0, gx::Framebuffer::Depth);
  if(fb_shadow.status() != gx::Framebuffer::Complete) {
    win32::panic("couldn't create shadowmap Framebuffer!", win32::FramebufferError);
  }

  fb_shadow.label("FB_shadow");

  gx::Framebuffer fb_resolve;

  fb_resolve.use()
    .renderbuffer(FB_DIMS.x, FB_DIMS.y, gx::rgb8, gx::Framebuffer::Color(0));
  if(fb_resolve.status() != gx::Framebuffer::Complete) {
    win32::panic("couldn't create MSAA resolve Framebuffer!", win32::FramebufferError);
  }

  fb_resolve.label("FB_resolve");

  auto shadow_pipeline = gx::Pipeline{}
    .viewport(0, 0, 1024, 1024)
    .depthTest(gx::Pipeline::LessEqual)
    .cull(gx::Pipeline::Front)
    .writeDepthOnly()
    .clearDepth(1.0f)
    ;

  auto pipeline = gx::Pipeline()
    .viewport(0, 0, FB_DIMS.x, FB_DIMS.y)
    .depthTest(gx::Pipeline::LessEqual)
    .cull(gx::Pipeline::Back)
    .clear(vec4{ 0.1f, 0.1f, 0.1f, 1.0f }, 1.0f);

  float r = 1280.0f;
  float b = 720.0f;

  mat4 ortho = xform::ortho(0, 0, b, r, 0.1f, 1000.0f);

  mat4 zoom_mtx = xform::identity();
  mat4 rot_mtx = xform::identity();

  window.captureMouse();

  int frames = 0;

  float old_fps;

  bool display_tex_matrix = false,
    ortho_projection = false;

  struct Vertex {
    vec3 pos, normal;
  };

  vec3 normals[] = {
    {0.0f, 0.0f, -1.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, -1.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f},
    {1.0f, 0.0f, 0.0f},
  };

  std::vector<Vertex> vtxs = {
    // BACK
    { { -1.0f, 1.0f, -1.0f }, normals[0] },
    { { 1.0f, -1.0f, -1.0f }, normals[0] },
    { { -1.0f, -1.0f, -1.0f }, normals[0] },

    { { 1.0f, -1.0f, -1.0f }, normals[0] },
    { { -1.0f, 1.0f, -1.0f }, normals[0] },
    { { 1.0f, 1.0f, -1.0f }, normals[0] },

    // FRONT
    { { -1.0f, 1.0f, 1.0f }, normals[1] },
    { { -1.0f, -1.0f, 1.0f }, normals[1] },
    { { 1.0f, -1.0f, 1.0f }, normals[1] },

    { { 1.0f, -1.0f, 1.0f }, normals[1] },
    { { 1.0f, 1.0f, 1.0f }, normals[1] },
    { { -1.0f, 1.0f, 1.0f }, normals[1] },
    
    // TOP
    { { -1.0f, 1.0f, -1.0f }, normals[2] },
    { { -1.0f, 1.0f, 1.0f }, normals[2] },
    { { 1.0f, 1.0f, 1.0f }, normals[2] },

    { { 1.0f, 1.0f, 1.0f }, normals[2] },
    { { 1.0f, 1.0f, -1.0f }, normals[2] },
    { { -1.0f, 1.0f, -1.0f }, normals[2] },
    
    // BOTTOM
    { { -1.0f, -1.0f, -1.0f }, normals[3] },
    { { 1.0f, -1.0f, 1.0f }, normals[3] },
    { { -1.0f, -1.0f, 1.0f }, normals[3] },

    { { 1.0f, -1.0f, 1.0f }, normals[3] },
    { { -1.0f, -1.0f, -1.0f }, normals[3] },
    { { 1.0f, -1.0f, -1.0f }, normals[3] },

    // LEFT
    { { -1.0f, 1.0f, -1.0f }, normals[4] },
    { { -1.0f, -1.0f, -1.0f }, normals[4] },
    { { -1.0f, -1.0f, 1.0f }, normals[4] },

    { { -1.0f, -1.0f, 1.0f }, normals[4] },
    { { -1.0f, 1.0f, 1.0f }, normals[4] },
    { { -1.0f, 1.0f, -1.0f }, normals[4] },

    // RIGHT
    { { 1.0f, 1.0f, -1.0f }, normals[5] },
    { { 1.0f, -1.0f, 1.0f }, normals[5] },
    { { 1.0f, -1.0f, -1.0f }, normals[5] },

    { { 1.0f, -1.0f, 1.0f }, normals[5] },
    { { 1.0f, 1.0f, -1.0f }, normals[5] },
    { { 1.0f, 1.0f, 1.0f }, normals[5] },
  };

  struct Light {
    vec4 position, color;
  };

  struct LightBlock {
    Light lights[4];
    int num_lights;
  };

  LightBlock light_block;

  gx::UniformBuffer light_ubo(gx::Buffer::Dynamic);

  light_ubo.label("UBO_lights");

  light_ubo.init(sizeof(LightBlock), 1);
  light_ubo.bindToIndex(0);

  program.uniformBlockBinding("LightBlock", 0);
  tex_program.uniformBlockBinding("LightBlock", 0);

  vec3 light_position[] = {
    { 0, 6, 0 },
    { -5, 6, 0 },
    { 5, 6, 0 },
  };

  std::vector<vec2> floor_vtxs = {
    { -1.0f, 1.0f },  { 0.0f, 1.0f },
    { -1.0f, -1.0f }, { 0.0f, 0.0f },
    { 1.0f, -1.0f },  { 1.0f, 0.0f },

    { 1.0f, -1.0f },  { 1.0f, 0.0f },
    { 1.0f, 1.0f },   { 1.0f, 1.0f },
    { -1.0f, 1.0f },  { 0.0f, 1.0f },

    { -1.0f, 1.0f },  { 0.0f, 1.0f },
    { 1.0f, -1.0f },  { 1.0f, 0.0f },
    { -1.0f, -1.0f }, { 0.0f, 0.0f },

    { 1.0f, -1.0f },  { 1.0f, 0.0f },
    { -1.0f, 1.0f },  { 0.0f, 1.0f },
    { 1.0f, 1.0f },   { 1.0f, 1.0f },

  };

  gx::VertexBuffer vbuf(gx::Buffer::Static);
  vbuf.init(vtxs.data(), vtxs.size());

  gx::VertexArray arr(fmt, vbuf);

  auto floor_fmt = gx::VertexFormat()
    .attr(gx::f32, 2)
    .attr(gx::f32, 2);

  gx::VertexBuffer floor_vbuf(gx::Buffer::Static);
  floor_vbuf.init(floor_vtxs.data(), floor_vtxs.size());

  gx::VertexArray floor_arr(floor_fmt, floor_vbuf);

  auto floor_sampler = gx::Sampler::repeat2d_mipmap()
    .param(gx::Sampler::Anisotropy, 16.0f);

  ui::Ui iface(ui::Geometry{ 0, 0, WindowSize.x, WindowSize.y }, ui::Style::basic_style());

  auto& layout = ui::create<ui::RowLayoutFrame>(iface)
    .frame<ui::PushButtonFrame>(iface, "b")
    .frame(ui::create<ui::ColumnLayoutFrame>(iface)
           .frame(ui::create<ui::LabelFrame>(iface).caption("Toggle texmatrix:"))
           .frame<ui::CheckBoxFrame>(iface, "e")
             .gravity(ui::Frame::Left))
    ;

  auto btn_b = iface.getFrameByName<ui::PushButtonFrame>("b");
  btn_b->caption("Quit Application").onClick([&](auto target) {
    window.quit();
  });

  auto& checkbox = iface.getFrameByName<ui::CheckBoxFrame>("e")->value(false);

  iface
    .frame(layout, { 30.0f, 500.0f })
    .frame(ui::create<ui::ConsoleFrame>(iface, "g_console").dropped(true))
    ;

  auto& console = *iface.getFrameByName<ui::ConsoleFrame>("g_console");

  console.onCommand([&](auto target, const char *command) {
    try {
      py::exec(command);
      console.print(">>> " + win32::StdStream::gets());
    } catch(py::Exception e) {
      std::string exception_type = e.type().attr("__name__").str();
      if(exception_type == "SystemExit") exit(0);

      console.print(exception_type);
      console.print(e.value().str());
    }
  });

  console.print(win32::StdStream::gets());

  auto fps_timer = win32::DeltaTimer();

  constexpr auto anim_time = 10000.0f;
  auto anim_timer = win32::LoopTimer().durationSeconds(2.5);

  while(window.processMessages()) {
    win32::Timers::tick();

    vec4 eye{ 0, 0, 60.0f/zoom, 1 };

    mat4 eye_mtx = xform::Transform()
      .translate(-pos)
      .rotx(-pitch)
      .roty(yaw)
      .translate(pos * 2)
      .matrix()
      ;
    eye = eye_mtx*eye;

    while(auto input = window.getInput()) {
      cursor.input(input);

      if(iface.input(cursor, input)) continue;

      if(auto kb = input->get<win32::Keyboard>()) {
        using win32::Keyboard;

        if(kb->keyDown('U')) {
          animate = animate < 0 ? win32::Timers::time_ms() : -1;
        } else if(kb->keyDown('N')) {
          normals[0] = normals[0]+vec3{ 0.05f, 0.05f, 0.05f };
        } else if(kb->keyDown('Q')) {
          window.quit();
        } else if(kb->keyDown('O')) {
          ortho_projection = !ortho_projection;
        } else if(kb->keyDown('W')) {
          pipeline.isEnabled(gx::Pipeline::Wireframe) ? pipeline.filledPolys() : pipeline.wireframe();
        } else if(kb->keyDown('`')) {
          console.toggle();
        }
      } else if(auto mouse = input->get<win32::Mouse>()) {
        using win32::Mouse;

        cursor.visible(!mouse->buttons);
        if(mouse->buttonDown(Mouse::Left)) iface.keyboard(nullptr);

        if(mouse->buttons & Mouse::Left) {
          mat4 d_mtx = xform::identity()
            *xform::roty(yaw)
            *xform::rotx(-pitch)
            ;
          vec4 d = d_mtx*vec4{ mouse->dx, -mouse->dy, 0, 1 };

          pos -= d.xyz() * (0.01f/zoom);
        } else if(mouse->buttons & Mouse::Right) {
          constexpr float factor = PIf/1024.0f;

          pitch += mouse->dy * factor;
          yaw += mouse->dx * factor;

          pitch = clamp(pitch, (-PIf/3.0f) + 0.01f, (PIf/3.0f) - 0.01f);
        } else if(mouse->event == Mouse::Wheel) {
          zoom += (mouse->ev_data/120)*0.05f;
        } else if(mouse->buttonDown(Mouse::Middle)) {
          zoom = 1.0f;

          zoom_mtx = xform::identity();
        }
      }
    }

    display_tex_matrix = checkbox.value();

    if(animate > 0) {
      rot_mtx = xform::translate(1280/2.0f, 720.0f/2.0f, 0)
        *xform::rotz(lerp(0.0f, 3.1415f, anim_timer.elapsedf()))
        *xform::translate(-1280.0f/2.0f, -720.0f/2.0f, 0)
        ;
    } else {
      rot_mtx = xform::identity();
    }

    mat4 model =
      xform::translate(pitch, yaw, -50.0f)
      *zoom_mtx
      *rot_mtx
      ;

    auto persp = !ortho_projection ? xform::perspective(70, (float)FB_DIMS.x/(float)FB_DIMS.y, 0.1f, 1000.0f) :
      xform::ortho(9.0f, -16.0f, -9.0f, 16.0f, 0.1f, 1000.0f)*xform::scale(zoom*2.0f);

    auto view = xform::look_at(sun, pos, vec3{ 0, 1, 0 });

    vec4 color;

    auto drawcube = [&]()
    {
      mat4 modelview = view*model;
      program.use()
        .uniformMatrix4x4(U.program.uProjection, persp)
        .uniformMatrix4x4(U.program.uModelView, modelview)
        .uniformMatrix3x3(U.program.uNormal, modelview.inverse().transpose())
        .uniformVector4(U.program.uCol, color)
        .draw(gx::Triangles, arr, vtxs.size());
    };

    shadow_pipeline.use();

    fb_shadow.use();
    gx::clear(gx::Framebuffer::DepthBit);

    model = xform::Transform()
      .translate(0.0f, 3.0f, 0.0f)
      .matrix();
    drawcube();

    model = xform::Transform()
      .translate(3.0f, 0.0f, -6.0f)
      .matrix();
    drawcube();

    model = xform::Transform()
      .translate(-3.0f, 0.0f, -6.0f)
      .matrix();
    drawcube();

    model = xform::identity()
      *xform::translate(0.0f, -1.01f, -6.0f)
      *xform::scale(10.0f)
      *xform::rotx(PIf/2.0f)
      ;

    mat4 modelview = view*model;

    gx::tex_unit(0, tex, floor_sampler);
    tex_program.use()
      .uniformMatrix4x4(U.tex.uProjection, persp)
      .uniformMatrix4x4(U.tex.uModelView, modelview)
      .uniformMatrix3x3(U.tex.uNormal, modelview.inverse().transpose())
      .uniformSampler(U.tex.uTex, 0)
      .draw(gx::Triangles, floor_arr, floor_vtxs.size());

    pipeline.use();
    gx::tex_unit(0, tex, sampler);

    fb.use();
    gx::clear(gx::Framebuffer::ColorBit|gx::Framebuffer::DepthBit);

    view = xform::look_at(eye.xyz(), pos, vec3{ 0, 1, 0 });

    light_block.num_lights = 3;

    light_block.lights[0] ={
      view*light_position[0],
      vec3{ 1.0f, 1.0f, 1.0f }
    };
    light_block.lights[1] ={
      view*light_position[1],
      vec3{ 1.0f, 1.0f, 1.0f }
    };
    light_block.lights[2] ={
      view*light_position[2],
      vec3{ 1.0f, 1.0f, 1.0f }
    };

    light_ubo.upload(&light_block, 0, 1);

    auto rot = xform::identity()
      *xform::roty(lerp(0.0, PI, anim_timer.elapsedf()))
      ;

    color = { 1.0f, 1.0f, 1.0f, -1.0f };
    model = xform::identity()
      *xform::translate(0.0f, 3.0f, 0.0f)
      *xform::scale(1.5f)
      ;
    drawcube();

    color = { 1.0f, 1.0f, 1.0f, -1.0f };
    model = xform::identity()
      *xform::translate(3.0f, 0.0f, -6.0f)
      *rot
      ;
    drawcube();

    color = { 1.0f, 0.5f, 1.0f, -1.0f };
    model = xform::identity()
      *xform::translate(-3.0f, 0.0f, -6.0f)
      *rot
      ;
    drawcube();

    model = xform::identity()
      *xform::translate(0.0f, -1.01f, -6.0f)
      *xform::scale(10.0f)
      *xform::rotx(PIf/2.0f)
      ;

    auto texmatrix = xform::Transform()
      .translate(-5.0f, -5.0f, 0.0f)
      .scale(1.0f/(sin(win32::Timers::timef_s() * PI/2.0) + 2.0f))
      .rotz(lerp(0.0f, PIf, anim_timer.elapsedf()))
      .translate(5.0f, 5.0f, 0.0f)
      .matrix()
      ;

    modelview = view*model;

    gx::tex_unit(0, tex, floor_sampler);
    tex_program.use()
      .uniformMatrix4x4(U.tex.uProjection, persp)
      .uniformMatrix4x4(U.tex.uModelView, modelview)
      .uniformMatrix3x3(U.tex.uNormal, modelview.inverse().transpose())
      .uniformMatrix4x4(U.tex.uTexMatrix, display_tex_matrix ? texmatrix : xform::identity())
      .uniformSampler(U.tex.uTex, 0)
      .draw(gx::Triangles, floor_arr, floor_vtxs.size());

    gx::Pipeline()
      .additiveBlend()
      .depthTest(gx::Pipeline::LessEqual)
      .use();
    for(int i = 0; i < light_block.num_lights; i++) {
      vec4 pos = light_position[i];
      color = { 0, 0, 0, (float)i };
      model = xform::Transform()
        .scale(0.2f)
        .translate(pos)
        .matrix()
        ;
      drawcube();

      vec2 screen = xform::project(vec3{ -1.5f, 1, -1 }, persp*view*model, FB_DIMS);
      screen.y -= 10;

      small_face.draw(util::fmt("Light %d", i+1), screen, { 1, 1, 1 });
    }

    int fps = (float)frames / fps_timer.elapsedSecondsf();

    constexpr float smoothing = 0.9f;
    old_fps = fps;
    fps = (float)old_fps*smoothing + (float)fps*(1.0f-smoothing);

    face.draw(util::fmt("FPS: %d", fps), vec2{ 30.0f, 70.0f }, vec4{ 0.8f, 0.0f, 0.0f, 1.0f });

    small_face.draw(util::fmt("anim_factor: %.2f eye: (%.2f, %.2f, %.2f)",
                              anim_timer.elapsedf(), eye.x, eye.y, eye.z),
                    { 30.0f, 100.0f }, { 1.0f, 1.0f, 1.0f });

    small_face.draw(util::fmt("time: %.8lfs", fps_timer.elapsedSecondsf()),
                    { 30.0f, 100.0f+small_face.height() }, { 1.0f, 1.0f, 1.0f });

    iface.paint();
    cursor.paint();

    fb.blit(fb_resolve, ivec4{ 0, 0, FB_DIMS.x, FB_DIMS.y }, ivec4{ 0, 0, FB_DIMS.x, FB_DIMS.y },
            gx::Framebuffer::ColorBit, gx::Sampler::Nearest);

    fb_resolve.blitToWindow(ivec4{ 0, 0, FB_DIMS.x, FB_DIMS.y }, ivec4{ 0, 0, (int)WindowSize.x, (int)WindowSize.y },
                    gx::Framebuffer::ColorBit, gx::Sampler::Linear);

    window.swapBuffers();

    frames++;
  }

  game::finalize();
  py::finalize();
  ui::finalize();
  ft::finalize();
  gx::finalize();

  win32::finalize();

  return 0;
}