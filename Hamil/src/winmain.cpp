#include <util/format.h>
#include <util/opts.h>

#include <math/geometry.h>
#include <math/xform.h>
#include <math/quaternion.h>
#include <math/transform.h>
#include <math/util.h>

#include <win32/win32.h>
#include <win32/panic.h>
#include <win32/cpuid.h>
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
#include <ui/window.h>
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

#include <bt/bullet.h>
#include <bt/world.h>
#include <bt/rigidbody.h>

#include <resources.h>
#include <res/res.h>
#include <res/manager.h>
#include <res/resource.h>
#include <res/handle.h>
#include <res/text.h>
#include <res/shader.h>
#include <res/image.h>

#include <mesh/util.h>

#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/entityman.h>
#include <hm/component.h>
#include <hm/componentman.h>
#include <hm/components/all.h>

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

  constexpr vec2 WindowSize       = { 1600, 900 };
  constexpr ivec2 FramebufferSize = { 1280, 720 };

  using win32::Window;
  Window window(WindowSize.x, WindowSize.y);

  gx::init();
  ft::init();
  ui::init();
  py::init();
  bt::init();
  res::init();
  hm::init();

  printf("%s\n", math::to_str(
    quat::from_euler(PIf, 0, 0)
    *quat::from_euler(0.0f, 0.0f, PIf/2.0f)
    *quat::from_euler(0.0f, PIf/2.0f, 0.0f) * vec3::down()).data());

  auto world = bt::DynamicsWorld();
  world.initDbgSimulation();

  res::load(R.shader.shaders.ids);
  res::load(R.image.ids);

  ft::Font face(ft::FontFamily("georgia"), 35);
  ft::Font small_face(ft::FontFamily("segoeui"), 12);

  ui::CursorDriver cursor(1280/2, 720/2);
  vec3 pos{ 0, 0, 0 };
  float pitch = 0, yaw = 0;
  float zoom = 1.0f, rot = 0.0f;

  vec3 sun { -120.0f, 160.0f, 140.0f };

  int animate = -1;

  auto fmt = gx::VertexFormat()
    .attr(gx::f32, 3)
    .attr(gx::f32, 3);

  res::Handle<res::Image> r_texture = R.image.tex;

  gx::Texture2D tex(gx::rgb);
  auto sampler = gx::Sampler::repeat2d_linear()
    .param(gx::Sampler::Anisotropy, 16.0f);

  tex.init(r_texture->data(), 0, r_texture->width(), r_texture->height(), gx::rgba, gx::u8);
  tex.generateMipmaps();

  byte cubemap_colors[][3] ={
    { 0x20, 0x20, 0x20, },
    { 0x20, 0x20, 0x20, },
    { 0x20, 0x20, 0x20, },
    { 0x00, 0x00, 0x00, },
    { 0x20, 0x20, 0x20, },
    { 0x20, 0x20, 0x20, },
  };

  gx::TextureCubeMap cubemap(gx::rgb);
  for(size_t i = 0; i < gx::Faces.size(); i++) {
    auto face  = gx::Faces[i];
    void *data = cubemap_colors[i];

    cubemap.init(data, 0, face, 1, gx::rgb, gx::u8);
  }

  cubemap.label("SKYBOX_cubemap");

  auto cubemap_sampler = gx::Sampler::repeat2d_linear();

  auto skybox_fmt = gx::VertexFormat()
    .attr(gx::f32, 3);

  auto skybox_mesh = mesh::box(1, 1, 1);

  auto& skybox_verts = std::get<0>(skybox_mesh);
  auto& skybox_inds  = std::get<1>(skybox_mesh);

  gx::VertexBuffer skybox_vbuf(gx::Buffer::Static);
  gx::IndexBuffer  skybox_ibuf(gx::Buffer::Static, gx::u16);
  skybox_vbuf.init(skybox_verts.data(), skybox_verts.size());
  skybox_ibuf.init(skybox_inds.data(), skybox_inds.size());

  gx::IndexedVertexArray skybox_arr(skybox_fmt, skybox_vbuf, skybox_ibuf);

  skybox_arr.label("SKYBOX");
  skybox_arr.end();

  res::Handle<res::Shader> r_phong = R.shader.shaders.phong,
    r_program = R.shader.shaders.program,
    r_tex = R.shader.shaders.tex,
    r_skybox = R.shader.shaders.skybox;

  auto program     = gx::make_program(
    r_program->source(res::Shader::Vertex), r_program->source(res::Shader::Fragment), U.program);
  auto tex_program = gx::make_program(
    r_tex->source(res::Shader::Vertex), r_tex->source(res::Shader::Fragment), U.tex);

  auto skybox_program = gx::make_program(
    r_skybox->source(res::Shader::Vertex), r_skybox->source(res::Shader::Fragment), U.skybox);

  program.label("program");
  tex_program.label("TEX_program");

  skybox_program.label("SKYBOX_program");

  gx::Texture2D fb_tex(gx::rgb8);
  gx::Framebuffer fb;

  fb_tex.initMultisample(2, FramebufferSize.x, FramebufferSize.y);
  //fb_tex.init(FramebufferSize.x, FramebufferSize.y);
  fb_tex.label("FB_tex");

  fb.use()
    .tex(fb_tex, 0, gx::Framebuffer::Color(0))
    .renderbuffer(gx::depth24, gx::Framebuffer::Depth);
  if(fb.status() != gx::Framebuffer::Complete) {
    win32::panic("couldn't create main Framebuffer!", win32::FramebufferError);
  }

  fb.label("FB");

  gx::Framebuffer fb_resolve;

  fb_resolve.use()
    .renderbuffer(FramebufferSize.x, FramebufferSize.y, gx::rgb8, gx::Framebuffer::Color(0));
  if(fb_resolve.status() != gx::Framebuffer::Complete) {
    win32::panic("couldn't create MSAA resolve Framebuffer!", win32::FramebufferError);
  }

  fb_resolve.label("FB_resolve");

  auto pipeline = gx::Pipeline()
    .viewport(0, 0, FramebufferSize.x, FramebufferSize.y)
    .depthTest(gx::Pipeline::LessEqual)
    .cull(gx::Pipeline::Back)
    .seamlessCubemap()
    .clear(vec4{ 0.1f, 0.1f, 0.1f, 1.0f }, 1.0f);

  float r = 1280.0f;
  float b = 720.0f;

  mat4 ortho = xform::ortho(0, 0, b, r, 0.1f, 1000.0f);

  mat4 zoom_mtx = xform::identity();

  window.captureMouse();

  float old_fps;

  bool ortho_projection = false;

  auto sphere = mesh::sphere(16, 16);

  auto& sphere_verts = std::get<0>(sphere);
  auto& sphere_inds  = std::get<1>(sphere);

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
    { -10, 6, -10 },
    { 20, 6, 0 },
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
  gx::IndexBuffer  ibuf(gx::Buffer::Static, gx::u16);

  vbuf.init(sphere_verts.data(), sphere_verts.size());
  ibuf.init(sphere_inds.data(), sphere_inds.size());

  gx::IndexedVertexArray arr(fmt, vbuf, ibuf);

  auto line_fmt = gx::VertexFormat()
    .attr(gx::f32, 3);

  auto line = mesh::box(0.05f, 0.5f, 0.05f);
  auto line_vtxs = std::get<0>(line);
  auto line_inds = std::get<1>(line);

  gx::VertexBuffer line_vbuf(gx::Buffer::Static);
  gx::IndexBuffer line_ibuf(gx::Buffer::Static, gx::u16);

  line_vbuf.init(line_vtxs.data(), line_vtxs.size());
  line_ibuf.init(line_inds.data(), line_inds.size());

  gx::IndexedVertexArray line_arr(line_fmt, line_vbuf, line_ibuf);

  auto floor_fmt = gx::VertexFormat()
    .attr(gx::f32, 2)
    .attr(gx::f32, 2);

  gx::VertexBuffer floor_vbuf(gx::Buffer::Static);
  floor_vbuf.init(floor_vtxs.data(), floor_vtxs.size());

  gx::VertexArray floor_arr(floor_fmt, floor_vbuf);

  auto floor_sampler = gx::Sampler::repeat2d_mipmap()
    .param(gx::Sampler::Anisotropy, 16.0f);

  ui::Ui iface(ui::Geometry{ 0, 0, WindowSize.x, WindowSize.y }, ui::Style::basic_style());

  iface.realSize(FramebufferSize.cast<float>());

  auto& layout = ui::create<ui::RowLayoutFrame>(iface)
    .frame<ui::PushButtonFrame>(iface, "b")
    .frame(ui::create<ui::HSliderFrame>(iface, "near")
            .range(1.0f, 100.0f))
    .frame(ui::create<ui::LabelFrame>(iface, "near_val")
            .caption(util::fmt("Near: %.2f  ", 0.0f))
            .gravity(ui::Frame::Center))
    ;

  auto btn_b = iface.getFrameByName<ui::PushButtonFrame>("b");
  btn_b->caption("Quit Application").onClick([&](auto target) {
    window.quit();
  });

  auto& near_slider = *iface.getFrameByName<ui::HSliderFrame>("near");
  auto& near_val = *iface.getFrameByName<ui::LabelFrame>("near_val");

  near_slider.onChange([&](ui::SliderFrame *target) {
    near_val.caption(util::fmt("Near: %.2lf", target->value()));
  });
  near_slider.value(1.0);

  iface
    .frame(ui::create<ui::WindowFrame>(iface)
      .title("Window")
      .content(layout)
      .position({ 30.0f, 500.0f }))
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
  auto anim_timer = win32::LoopTimer().durationSeconds(2.5);
  auto step_timer = win32::DeltaTimer();
  auto nudge_timer = win32::DeltaTimer();

  unsigned num_spheres = 0;

  step_timer.reset();
  nudge_timer.stop();

  while(window.processMessages()) {
    win32::Timers::tick();

    vec4 eye{ 0, 0, 60.0f/zoom, 1 };

    mat4 eye_mtx = xform::Transform()
      .translate(-pos)
      .rotx(-pitch)
      .roty(yaw)
      .translate(pos * 2.0f)
      .matrix()
      ;
    eye = eye_mtx*eye;

    float nudge_force = 0.0f;

    while(auto input = window.getInput()) {
      cursor.input(input);

      if(iface.input(cursor, input)) continue;

      if(auto kb = input->get<win32::Keyboard>()) {
        using win32::Keyboard;

        if(kb->keyDown('S')) {
          world.startDbgSimulation();
        } else if(kb->keyDown('Q')) {
          window.quit();
        } else if(kb->keyDown('O')) {
          ortho_projection = !ortho_projection;
        } else if(kb->keyDown('D')) {
          auto name = util::fmt("sphere%u", num_spheres);
          auto entity = hm::entities().createGameObject(name);
          auto body = world.createDbgSimulationRigidBody({ 0.0f, 10.0f, 0.0f });

          entity.addComponent<hm::RigidBody>(body);

          world.addRigidBody(body);

          num_spheres++;
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
          zoom = clamp(zoom+(mouse->ev_data/120)*0.05f, 0.01f, INFINITY);
        } else if(mouse->buttonDown(Mouse::Middle)) {
          zoom = 1.0f;

          zoom_mtx = xform::identity();
        }

        if(mouse->buttonDown(Mouse::Left)) {
          nudge_timer.reset();
        } else if(mouse->buttonUp(Mouse::Left)) {
          nudge_force = nudge_timer.elapsedSecondsf();
          nudge_timer.stop();
        }
      }
    }

    mat4 model =
      xform::translate(pitch, yaw, -50.0f)
      *zoom_mtx
      ;

    auto persp = !ortho_projection ?
      xform::perspective(70, (float)FramebufferSize.x/(float)FramebufferSize.y, near_slider.value(), 10e20f) :
      xform::ortho(9.0f, -16.0f, -9.0f, 16.0f, 10.0f, 10e20f)*xform::scale(zoom*2.0f);

    auto view = xform::look_at(sun, pos, vec3{ 0, 1, 0 });

    vec4 color;

    size_t num_tris = 0;

    auto drawsphere = [&]()
    {
      mat4 modelview = view*model;
      program.use()
        .uniformMatrix4x4(U.program.uProjection, persp)
        .uniformMatrix4x4(U.program.uModelView, modelview)
        .uniformMatrix3x3(U.program.uNormal, modelview.inverse().transpose())
        .uniformVector4(U.program.uCol, color)
        .draw(gx::Triangles, arr, sphere_inds.size());
      arr.end();

      num_tris += sphere_inds.size() / 3;
    };

    mat4 modelview = xform::identity();

    pipeline.use();
    gx::tex_unit(0, tex, sampler);

    fb.use();
    gx::clear(gx::Framebuffer::ColorBit|gx::Framebuffer::DepthBit);

    view = xform::look_at(eye.xyz(), pos, vec3{ 0, 1, 0 });

    light_block.num_lights = 3;

    light_block.lights[0] = {
      view*vec4(light_position[0], 1.0f),
      vec3{ 1.0f, 0.0f, 0.0f }
    };
    light_block.lights[1] = {
      view*vec4(light_position[1], 1.0f),
      vec3{ 0.0f, 1.0f, 0.0f }
    };
    light_block.lights[2] = {
      view*vec4(light_position[2], 1.0f),
      vec3{ 0.0f, 0.0f, 1.0f }
    };

    light_ubo.upload(&light_block, 0, 1);

    auto rot = xform::identity()
      *xform::roty(lerp(0.0, PI, anim_timer.elapsedf()))
      ;

    vec4 mouse_ray = xform::unproject({ cursor.pos(), 0.5f }, persp*view, FramebufferSize);
    vec3 mouse_ray_direction = vec4::direction(eye, mouse_ray).xyz();

    bt::RigidBody picked_body;
    vec3 hit_normal;
    if(mouse_ray.w != 0.0f) {
      auto to = eye.xyz() + mouse_ray_direction*10e10f;

      picked_body = world.pickDbgSimulation(eye.xyz(), to, hit_normal);
    }

    if(picked_body && picked_body.hasMotionState()) {
      float force_factor = 1.0f + pow(nudge_timer.elapsedSecondsf(), 3.0f);

      auto q = quat::rotation_between(vec3::up(), hit_normal);

      model = xform::Transform()
        .translate(0.0f, 0.5f, 0.0f)
        .scale(1.0f, 1.5f*force_factor, 1.0f)
        .rotate(q)
        .translate(picked_body.origin())
        .matrix();

      program.use()
        .uniformMatrix4x4(U.program.uProjection, persp)
        .uniformMatrix4x4(U.program.uModelView, view*model)
        .uniformVector4(U.program.uCol, { 0, 0, 0, 0.0f })
        .draw(gx::Triangles, line_arr, line_inds.size());
      line_arr.end();
    }

    if(nudge_force > 0.0f && picked_body) {
      auto center_of_mass = picked_body.centerOfMass();
      float force_factor = 1.0f + pow(nudge_force, 3.0f)*10.0f;

      picked_body
        .activate()
        .applyImpulse(-hit_normal*force_factor, center_of_mass);
    }

    world.stepDbgSimulation(step_timer.elapsedSecondsf());
    hm::components().foreach([&](hm::ComponentRef<hm::RigidBody> component) {
      bt::RigidBody rb = component().rb;

      if(rb.origin().distance2(vec3::zero()) > 10e3*10e3) {
        world.removeRigidBody(rb);
        component().entity().destroy();
        return;
      }

      color = { rb == picked_body ? vec3(1.0f, 0.0f, 0.0f) : vec3(1.0f), -1.0f };

      model = rb.worldTransformMatrix();
      drawsphere();
    });

    model = xform::identity()
      *xform::translate(0.0f, -1.01f, -6.0f)
      *xform::scale(50.0f)
      *xform::rotx(PIf/2.0f)
      ;

    auto texmatrix = xform::Transform()
      .scale(3.0f)
      .matrix()
      ;

    modelview = view*model;

    gx::tex_unit(0, tex, floor_sampler);
    tex_program.use()
      .uniformMatrix4x4(U.tex.uProjection, persp)
      .uniformMatrix4x4(U.tex.uModelView, modelview)
      .uniformMatrix3x3(U.tex.uNormal, modelview.inverse().transpose())
      .uniformMatrix4x4(U.tex.uTexMatrix, texmatrix)
      .uniformSampler(U.tex.uTex, 0)
      .draw(gx::Triangles, floor_arr, floor_vtxs.size());
    num_tris += floor_vtxs.size() / 3;

    for(int i = 0; i < light_block.num_lights; i++) {
      vec4 pos = light_position[i];
      color = { 0, 0, 0, (float)i };
      model = xform::Transform()
        .scale(0.2f)
        .translate(pos)
        .matrix()
        ;
      drawsphere();

      vec2 screen = xform::project(vec3{ -1.5f, 1, -1 }, persp*view*model, FramebufferSize);
      screen.y -= 10;

      small_face.draw(util::fmt("Light %d", i+1), screen, { 1, 1, 1 });
    }

    gx::tex_unit(1, cubemap, cubemap_sampler);

    gx::Pipeline()
      .depthTest(gx::Pipeline::LessEqual)
      .noCull()
      .use();

    skybox_program.use()
      .uniformMatrix4x4(U.skybox.uProjection, persp)
      .uniformMatrix4x4(U.skybox.uView, view)
      .uniformSampler(U.skybox.uEnvironmentMap, 1)
      .draw(gx::Triangles, skybox_arr, skybox_inds.size())
      ;
    skybox_arr.end();

    float fps = 1.0f / step_timer.elapsedSecondsf();

    constexpr float smoothing = 0.9f;
    old_fps = fps;
    fps = old_fps*smoothing + fps*(1.0f-smoothing);

    face.draw(util::fmt("FPS: %.2f", fps),
      vec2{ 30.0f, 70.0f }, { 0.8f, 0.0f, 0.0f });

    small_face.draw(util::fmt("Traingles: %zu", num_tris),
      { 30.0f, 70.0f+small_face.height() }, { 1.0f, 1.0f, 1.0f });

    if(picked_body) {
      auto entity = picked_body.user<hm::Entity>();

      small_face.draw(util::fmt("picked(0x%.8x) at: %s",
        entity.id(), math::to_str(picked_body.origin())),
        { 30.0f, 100.0f+small_face.height() }, { 1.0f, 1.0f, 1.0f });
    }

    float y = 150.0f;
    hm::components().foreach([&](hm::ComponentRef<hm::GameObject> component) {
      hm::Entity entity = component().entity();
      bt::RigidBody rb = entity.component<hm::RigidBody>().get().rb;

      small_face.draw(util::fmt("%s(0x%.8x) at: %s",
        entity.gameObject().name(), entity.id(), math::to_str(rb.origin())),
        { 30.0f, y }, { 1.0f, 1.0f, 1.0f });
      y += small_face.height();
    });

    iface.paint();
    cursor.paint();

    fb.copy(fb_resolve, ivec4{ 0, 0, FramebufferSize.x, FramebufferSize.y },
            gx::Framebuffer::ColorBit, gx::Sampler::Nearest);

    fb_resolve.blitToWindow(
      ivec4{ 0, 0, FramebufferSize.x, FramebufferSize.y },
      ivec4{ 0, 0, (int)WindowSize.x, (int)WindowSize.y },
      gx::Framebuffer::ColorBit, gx::Sampler::Linear);

    window.swapBuffers();

    step_timer.reset();
  }

  hm::finalize();
  res::finalize();
  bt::finalize();
  py::finalize();
  ui::finalize();
  ft::finalize();
  gx::finalize();

  win32::finalize();

  return 0;
}
