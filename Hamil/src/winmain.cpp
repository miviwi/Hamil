#include <util/format.h>
#include <util/opts.h>
#include <util/unit.h>
#include <util/dds.h>

#include <math/geometry.h>
#include <math/xform.h>
#include <math/quaternion.h>
#include <math/transform.h>
#include <math/util.h>
#include <math/frustum.h>

#include <win32/win32.h>
#include <win32/panic.h>
#include <win32/cpuid.h>
#include <win32/cpuinfo.h>
#include <win32/window.h>
#include <win32/time.h>
#include <win32/file.h>
#include <win32/stdstream.h>
#include <win32/mman.h>
#include <win32/thread.h>
#include <win32/mutex.h>
#include <win32/event.h>
#include <win32/conditionvar.h>

#include <sched/scheduler.h>
#include <sched/pool.h>
#include <sched/job.h>

#include <uniforms.h>
#include <gx/gx.h>
#include <gx/info.h>
#include <gx/buffer.h>
#include <gx/vertex.h>
#include <gx/program.h>
#include <gx/pipeline.h>
#include <gx/texture.h>
#include <gx/framebuffer.h>
#include <gx/renderpass.h>
#include <gx/resourcepool.h>
#include <gx/memorypool.h>
#include <gx/commandbuffer.h>
#include <gx/fence.h>
#include <gx/query.h>

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
#include <ui/scrollframe.h>

#include <py/python.h>
#include <py/object.h>
#include <py/exception.h>
#include <py/types.h>
#include <py/collections.h>
#include <py/modules/btmodule.h>

#include <yaml/document.h>
#include <yaml/node.h>

#include <bt/bullet.h>
#include <bt/world.h>
#include <bt/rigidbody.h>
#include <bt/ray.h>

#include <resources.h>
#include <res/res.h>
#include <res/manager.h>
#include <res/resource.h>
#include <res/handle.h>
#include <res/text.h>
#include <res/shader.h>
#include <res/image.h>
#include <res/texture.h>
#include <res/mesh.h>

#include <mesh/util.h>
#include <mesh/halfedge.h>
#include <mesh/obj.h>

#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/entityman.h>
#include <hm/component.h>
#include <hm/componentman.h>
#include <hm/components/all.h>

#include <ek/euklid.h>
#include <ek/renderer.h>
#include <ek/renderview.h>
#include <ek/renderobject.h>
#include <ek/visibility.h>
#include <ek/visobject.h>
#include <ek/occlusion.h>

#include <cli/cli.h>

#include <vector>
#include <array>
#include <utility>
#include <atomic>
#include <random>

//int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
int main(int argc, char *argv[])
{
  if(argc > 1) {
    if(auto exit_code = cli::args(argc, argv)) exit(exit_code);
  }

  win32::init();

  constexpr ivec2 WindowSize      = { 1600, 900 };
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
  ek::init();

  printf("extension(EXT::TextureSRGB):      %i\n", gx::info().extension(gx::EXT::TextureSRGB));
  printf("extension(ARB::ComputeShader):    %i\n", gx::info().extension(gx::ARB::ComputeShader));
  printf("extension(ARB::BindlessTexture):  %i\n", gx::info().extension(gx::ARB::BindlessTexture));
  printf("extension(ARB::TextureBPTC):      %i\n", gx::info().extension(gx::ARB::TextureBPTC));

  auto& pool = ek::renderer().pool();
  gx::MemoryPool memory(4096);

  sched::WorkerPool worker_pool;
  worker_pool
    .acquireWorkerGlContexts(window)
    .kickWorkers();

  std::random_device dev_random;
  std::mt19937 random_generator;
  std::uniform_real_distribution<float> random_floats(0.0f, 1.0f);

  random_generator.seed(dev_random());

  ek::renderer().cachePrograms();

  auto world = bt::DynamicsWorld();

  py::set_global("world", py::DynamicsWorld_FromDynamicsWorld(world));

//  res::load(R.image.ids);
  res::load(R.mesh.ids);

  auto bunny_fmt = gx::VertexFormat()
    .attr(gx::f32, 3)
    .attr(gx::f32, 3)
    .attr(gx::f32, 2);
    //.attrAlias(0, gx::f32, 2);

  auto bunny_vbuf_id = pool.createBuffer<gx::VertexBuffer>("bvBunny",
    gx::Buffer::Static);
  auto bunny_ibuf_id = pool.createBuffer<gx::IndexBuffer>("biBunny",
    gx::Buffer::Static, gx::u16);

  auto bunny_vbuf = pool.getBuffer(bunny_vbuf_id);
  auto bunny_ibuf = pool.getBuffer(bunny_ibuf_id);

  auto bunny_arr_id = pool.create<gx::IndexedVertexArray>("iaBunny",
    bunny_fmt, bunny_vbuf.get<gx::VertexBuffer>(), bunny_ibuf.get<gx::IndexBuffer>());
  auto& bunny_arr = pool.get<gx::IndexedVertexArray>(bunny_arr_id);

  res::Handle<res::Mesh> r_model = R.mesh.autumn_plains,
    r_model_hull = R.mesh.monkey_cube_hulls;

  auto& obj_loader = (mesh::ObjLoader&)r_model->loader();
  auto& obj_hull_loader = (mesh::ObjLoader&)r_model_hull->loader();

  auto model_load_job = obj_loader.streamIndexed(bunny_fmt, bunny_vbuf, bunny_ibuf);
  auto model_load_job_id = worker_pool.scheduleJob(model_load_job.get());

  ft::Font face(ft::FontFamily("georgia"), 35);
  ft::Font small_face(ft::FontFamily("segoeui"), 12);
  auto monospace_face_ptr = std::make_shared<ft::Font>(ft::FontFamily("consola"), 12, &pool);

  ui::CursorDriver cursor(1280/2, 720/2);
  vec3 pos{ 0, 0, 0 };
  float pitch = 0, yaw = 0;
  float zoom = 1.0f, rot = 0.0f;

  vec3 sun { -120.0f, 160.0f, 140.0f };

  int animate = -1;

  auto fmt = gx::VertexFormat()
    .attr(gx::f32, 3)
    .attr(gx::f32, 3)
    .attrAlias(0, gx::f32, 2);

  res::Handle<res::Texture> r_texture = R.texture.autumn_plains_tex;
  const auto& desk = r_texture->get();

  auto tex_id = pool.createTexture<gx::Texture2D>("t2dFloor", desk.texInternalFormat());
  auto& tex = pool.getTexture<gx::Texture2D>(tex_id);
  auto floor_sampler_id = pool.create<gx::Sampler>("sFloor",
    gx::Sampler::borderclamp2d());

  tex.init(desk.image().getData(), 0, desk.image().size2d(), desk.image().sz);
  tex.generateMipmaps();

  byte cubemap_colors[][3] = {
    { 0x00, 0x20, 0x20, }, { 0x20, 0x20, 0x00, },
    { 0x20, 0x20, 0x20, }, { 0x00, 0x00, 0x00, },
    { 0x20, 0x20, 0x20, }, { 0x20, 0x20, 0x20, },

#if 0
    vec4(0.0f, 0.25f, 0.25f, 1.0f), vec4(0.5f, 0.5f, 0.0f, 1.0f),
    vec4(0.25f, 0.25f, 0.25f, 1.0f), vec4(0.4f, 0.4f, 0.4f, 1.0f),
    vec4(0.1f, 0.1f, 0.1f, 1.0f), vec4(0.1f, 0.1f, 0.1f, 1.0f),
#endif
  };

  auto cubemap_id = pool.createTexture<gx::TextureCubeMap>("tcSkybox", gx::rgb);
  auto& cubemap = pool.getTexture<gx::TextureCubeMap>(cubemap_id);

  for(size_t i = 0; i < gx::Faces.size(); i++) {
    auto face  = gx::Faces[i];
    void *data = cubemap_colors[i];

    cubemap.init(data, 0, face, 1, gx::rgb, gx::u8);
  }

  auto cubemap_sampler_id = pool.create<gx::Sampler>("sSkybox",
    gx::Sampler::repeat2d_linear());

  auto skybox_fmt = gx::VertexFormat()
    .attr(gx::f32, 3);

  auto skybox_mesh = mesh::box(1, 1, 1);

  auto& skybox_verts = std::get<0>(skybox_mesh);
  auto& skybox_inds  = std::get<1>(skybox_mesh);

  gx::VertexBuffer skybox_vbuf(gx::Buffer::Static);
  gx::IndexBuffer  skybox_ibuf(gx::Buffer::Static, gx::u16);
  skybox_vbuf.init(skybox_verts.data(), skybox_verts.size());
  skybox_ibuf.init(skybox_inds.data(), skybox_inds.size());

  auto skybox_arr_id = pool.create<gx::IndexedVertexArray>("iaSkybox",
    skybox_fmt, skybox_vbuf, skybox_ibuf);
  auto& skybox_arr = pool.get<gx::IndexedVertexArray>(skybox_arr_id);

  std::vector<vec2> fullscreen_quad = {
    { -1.0f,  1.0f },
    { -1.0f, -1.0f },
    {  1.0f, -1.0f },
    {  1.0f,  1.0f },
  };

  auto fullscreen_quad_fmt = gx::VertexFormat()
    .attr(gx::f32, 2);
  
  gx::VertexBuffer fullscreen_quad_vbuf(gx::Buffer::Static);
  fullscreen_quad_vbuf.init(fullscreen_quad.data(), fullscreen_quad.size());

  auto fullscreen_quad_arr_id = pool.create<gx::VertexArray>("aFullscreenQuad",
    fullscreen_quad_fmt, fullscreen_quad_vbuf);
  auto& fullscreen_quad_arr = pool.get<gx::VertexArray>(fullscreen_quad_arr_id);

  res::load(R.shader.shaders.colorspace);
  res::load(R.shader.shaders.skybox);
  res::load(R.shader.shaders.composite);

  res::Handle<res::Shader>
    r_skybox = R.shader.shaders.skybox,
    r_composite = R.shader.shaders.composite;

  auto skybox_program_id = pool.create<gx::Program>(gx::make_program("pSkybox",
    r_skybox->source(res::Shader::Vertex), r_skybox->source(res::Shader::Fragment), U.skybox));
  auto& skybox_program = pool.get<gx::Program>(skybox_program_id);

  auto composite_program_id = pool.create<gx::Program>(gx::make_program("pComposite",
    r_composite->source(res::Shader::Vertex), r_composite->source(res::Shader::Fragment), U.composite));
  auto& composite_program = pool.get<gx::Program>(composite_program_id);

  gx::Texture2D occlusion_tex(gx::r16f);
  occlusion_tex.label("t2dOcclusionBuffer");
  occlusion_tex.init(ek::OcclusionBuffer::Size);

  gx::Framebuffer occlusion_fb;
  occlusion_fb.use()
    .tex(occlusion_tex, 0, gx::Framebuffer::Color(0));

  auto fb_composite_id = pool.create<gx::Framebuffer>("fbComposite");
  auto& fb_composite = pool.get<gx::Framebuffer>(fb_composite_id);

  fb_composite.use()
    .renderbuffer(FramebufferSize, gx::rgb8, gx::Framebuffer::Color(0), "rbComposite");
  if(fb_composite.status() != gx::Framebuffer::Complete) {
    win32::panic("couldn't create composite Framebuffer!", win32::FramebufferError);
  }

  auto resolve_sampler_id = pool.create<gx::Sampler>(gx::Sampler::borderclamp2d());

  struct SkyboxUniforms {
    mat4 view;
    mat4 persp;
  };

  auto skybox_uniforms_handle = memory.alloc<SkyboxUniforms>();
  auto& skybox_uniforms = *memory.ptr<SkyboxUniforms>(skybox_uniforms_handle);

  static constexpr uint AoKernelBinding = 3;

  const uint ubo_block_alignment = pow2_round((uint)gx::info().minUniformBindAlignment());
  auto ubo_align = [&](uint sz) {
    return pow2_align(sz, ubo_block_alignment);
  };

  skybox_program.use()
    .uniformSampler(U.skybox.uEnvironmentMap, 1);

  auto composite_pass_id = pool.create<gx::RenderPass>();
  auto& composite_pass = pool.get<gx::RenderPass>(composite_pass_id)
    .framebuffer(fb_composite_id)
    .pipeline(gx::Pipeline()
      .viewport(0, 0, FramebufferSize.x, FramebufferSize.y)
      .noDepthTest()
      .clear(vec4{ 0.0f, 0.0f, 0.0f, 1.0f, }, 1.0f))
    .clearOp(gx::RenderPass::ClearColor)
    ;

  float r = 1280.0f;
  float b = 720.0f;

  mat4 ortho = xform::ortho(0, 0, b, r, 0.1f, 1000.0f);

  mat4 zoom_mtx = mat4::identity();

  window.captureMouse();

  float old_fps;

  bool ortho_projection = false;

  auto sphere = mesh::sphere(16, 16);

  auto& sphere_verts = std::get<0>(sphere);
  auto& sphere_inds  = std::get<1>(sphere);

  vec3 light_position[] = {
    { 0, 6, 0 },
    { -10, 6, -10 },
    { 20, 6, 0 },
  };

  struct FloorVtx {
    vec3 pos;
    vec3 normal;
    vec2 tex_coord;
  };

  std::vector<FloorVtx> floor_vtxs = {
    { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
    { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
    { {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },

    { {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
    { {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
    { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },

    { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
    { {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
    { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },

    { {  1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
    { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
    { {  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
  };

  auto floor_fmt = gx::VertexFormat()
    .attr(gx::f32, 3)
    .attr(gx::f32, 3)
    .attr(gx::f32, 2);

  gx::VertexBuffer floor_vbuf(gx::Buffer::Static);
  floor_vbuf.init(floor_vtxs.data(), floor_vtxs.size());

  auto floor_arr_id = pool.create<gx::VertexArray>("aFloor", floor_fmt, floor_vbuf);
  auto& floor_arr = pool.get<gx::VertexArray>(floor_arr_id);

  gx::VertexBuffer sphere_vbuf(gx::Buffer::Static);
  gx::IndexBuffer  sphere_ibuf(gx::Buffer::Static, gx::u16);

  sphere_vbuf.init(sphere_verts.data(), sphere_verts.size());
  sphere_ibuf.init(sphere_inds.data(), sphere_inds.size());

  auto sphere_arr_id = pool.create<gx::IndexedVertexArray>("iaSphere",
    fmt, sphere_vbuf, sphere_ibuf);
  auto& sphere_arr = pool.get<gx::IndexedVertexArray>(sphere_arr_id);

  auto line_fmt = gx::VertexFormat()
    .attr(gx::f32, 3)
    .attrAlias(0, gx::f32, 3)
    .attrAlias(0, gx::f32, 2);

  auto line = mesh::box(0.5f, 0.5f, 0.5f);
  auto line_vtxs = std::get<0>(line);
  auto line_inds = std::get<1>(line);

  gx::VertexBuffer line_vbuf(gx::Buffer::Static);
  gx::IndexBuffer line_ibuf(gx::Buffer::Static, gx::u16);

  line_vbuf.init(line_vtxs.data(), line_vtxs.size());
  line_ibuf.init(line_inds.data(), line_inds.size());

  auto line_arr_id = pool.create<gx::IndexedVertexArray>("iaLine",
    line_fmt, line_vbuf, line_ibuf);
  ui::Ui iface(pool, ui::Geometry(vec2(), WindowSize.cast<float>()), ui::Style::basic_style());

  composite_pass.texture(5, iface.framebufferTextureId(), resolve_sampler_id);

  iface.realSize(FramebufferSize.cast<float>());

  auto& layout = ui::create<ui::RowLayoutFrame>(iface)
    .frame<ui::PushButtonFrame>(iface, "b")
    .frame(ui::create<ui::HSliderFrame>(iface, "exp")
      .range(0.1f, 10.0f)
      .step(0.1))
    .frame(ui::create<ui::LabelFrame>(iface, "exp_val")
      .caption(util::fmt("Exposure: %.1f  ", 0.0f))
      .padding({ 120.0f, 0.0f })
      .gravity(ui::Frame::Center))
    .frame(ui::create<ui::HSliderFrame>(iface, "ao_r")
      .range(0.0f, 5.0f))
    .frame(ui::create<ui::LabelFrame>(iface, "ao_r_val")
      .caption(util::fmt("AO radius: %.4f  ", 0.0f))
      .padding({ 120.0f, 0.0f })
      .gravity(ui::Frame::Center))
    .frame(ui::create<ui::HSliderFrame>(iface, "ao_bias")
      .range(0.0f, 1.0f))
    .frame(ui::create<ui::LabelFrame>(iface, "ao_bias_val")
      .caption(util::fmt("AO bias: %.4f  ", 0.0f))
      .padding({ 120.0f, 0.0f })
      .gravity(ui::Frame::Center))
    ;

  auto btn_b = iface.getFrameByName<ui::PushButtonFrame>("b");
  btn_b->caption("Quit Application").onClick([&](auto target) {
    window.quit();
  });

  auto& exp_slider = *iface.getFrameByName<ui::HSliderFrame>("exp");
  auto& exp_val = *iface.getFrameByName<ui::LabelFrame>("exp_val");

  exp_slider.onChange([&](ui::SliderFrame *target) {
    exp_val.caption(util::fmt("Exposure: %.1lf", target->value()));
  });
  exp_slider.value(1.0f);

  auto& ao_r_slider = *iface.getFrameByName<ui::HSliderFrame>("ao_r");
  auto& ao_r_val = *iface.getFrameByName<ui::LabelFrame>("ao_r_val");

  ao_r_slider.onChange([&](ui::SliderFrame *target) {
    ao_r_val.caption(util::fmt("AO radius: %.4lf", target->value()));
  });
  ao_r_slider.value(0.5);

  auto& ao_bias_slider = *iface.getFrameByName<ui::HSliderFrame>("ao_bias");
  auto& ao_bias_val = *iface.getFrameByName<ui::LabelFrame>("ao_bias_val");

  ao_bias_slider.onChange([&](ui::SliderFrame *target) {
    ao_bias_val.caption(util::fmt("AO bias: %.4lf", target->value()));
  });
  ao_bias_slider.value(0.1);

  auto& stats_layout = ui::create<ui::ScrollFrame>(iface).content(
    ui::create<ui::RowLayoutFrame>(iface)
      .frame(ui::create<ui::LabelFrame>(iface, "stats")
              .gravity(ui::Frame::Left)))
      .size({ 170.0f, 50.0f })
    ;

  auto& stats = *iface.getFrameByName<ui::LabelFrame>("stats");

  iface
    .frame(ui::create<ui::WindowFrame>(iface)
      .title("Window")
      .content(ui::create<ui::ScrollFrame>(iface)
        .content(layout)
        .size({ 150.0f, 100.0f }))
      .position({ 30.0f, 500.0f }))
    .frame(ui::create<ui::WindowFrame>(iface)
      .title("Statistics")
      .content(stats_layout)
      .position({ 1000.0f, 100.0f }))
    .frame(ui::create<ui::ConsoleFrame>(iface, "g_console"))
    ;

  auto& console = *iface.getFrameByName<ui::ConsoleFrame>("g_console");

  console.onCommand([&](auto target, const char *command) {
    try {
      py::exec(command);
      console.print(">>> " + win32::StdStream::gets());
    } catch(py::Exception e) {
      std::string exception_type = e.type().name();
      if(exception_type == "SystemExit") exit(0);

      console.print(exception_type);
      console.print(e.value().str());
    }
  });

  auto fps_timer = win32::DeltaTimer();
  auto anim_timer = win32::LoopTimer().durationSeconds(2.5);
  auto step_timer = win32::DeltaTimer();
  auto nudge_timer = win32::DeltaTimer();

  step_timer.reset();
  nudge_timer.stop();

  auto scene = hm::entities().createGameObject("Scene");

  scene.addComponent<hm::Transform>(xform::Transform());

  const int PboSize = sizeof(u8)*3 * FramebufferSize.area();

  auto pbo_id = pool.createBuffer<gx::PixelBuffer>("bpTest",
    gx::Buffer::DynamicRead, gx::PixelBuffer::Download);
  auto& pbo = pool.getBuffer<gx::PixelBuffer>(pbo_id);
  pbo.init(1, PboSize);

  auto floor_shape = bt::shapes().box({ 50.0f, 0.5f, 50.0f });
  auto floor_mesh = mesh::Mesh()
    .withNormals()
    .withTexCoords(1)
    .withArray(floor_arr_id)
    .withNum((u32)floor_vtxs.size());
  auto create_floor = [&]()
  {
    vec3 origin = { 0.0f, -1.5f, -6.0f };
    auto body = bt::RigidBody::create(floor_shape, origin)
      .rollingFriction(0.2f);

    auto floor = hm::entities().createGameObject("floor", scene);

    auto transform = floor.addComponent<hm::Transform>(
      origin + vec3{ 0.0f, 0.5f, 0.0f },
      //quat::from_euler(PIf/2.0f, 0.0f, 0.0f),
      quat::from_axis(vec3::right(), PIf/2.0f),
      vec3(50.0f),
      body.aabb()
    );
    floor.addComponent<hm::RigidBody>(body);
    floor.addComponent<hm::Mesh>(floor_mesh);

    auto material = floor.addComponent<hm::Material>();

    material().diff_type = hm::Material::DiffuseTexture;

    material().diff_tex.id = tex_id;
    material().diff_tex.sampler_id = floor_sampler_id;

    material().metalness = 0.0f;
    material().roughness = 0.4f;
    material().ior = vec3(0.01f);

    world.addRigidBody(body);

    auto vis = floor.addComponent<hm::Visibility>();

    ek::VisibilityMesh vis_mesh;
    vis_mesh.model = transform().t.matrix();
    vis_mesh.aabb = AABB(body.aabb().min - origin, body.aabb().max - origin);
    vis_mesh.num_verts = vis_mesh.num_inds = (u32)floor_vtxs.size();

    vis_mesh.verts = StridePtr<const vec3>(floor_vtxs.data(), sizeof(FloorVtx));

    u16 *inds = new u16[floor_vtxs.size()];
    for(int i = 0; i < floor_vtxs.size(); i++) inds[i] = i;

    vis_mesh.inds = inds;

    vis().vis.addMesh(std::move(vis_mesh));
    vis().vis.flags(ek::VisibilityObject::Occluder);

    return floor;
  };

  auto sphere_shape = bt::shapes().sphere(1.0f);
  auto sphere_mesh = mesh::Mesh()
    .withNormals()
    .withIndexedArray(sphere_arr_id)
    .withNum((u32)sphere_inds.size());
  unsigned num_spheres = 0;
  auto create_sphere = [&]()
  {
    vec3 origin = { 0.0f, 10.0f, 0.0f };
    auto body = bt::RigidBody::create(sphere_shape, origin, 1.0f);

    auto name = util::fmt("sphere%u", num_spheres);
    auto entity = hm::entities().createGameObject(name, scene);

    auto transform = entity.addComponent<hm::Transform>(
      body.worldTransform(),
      AABB(origin - vec3(1.0f), origin + vec3(1.0f))
    );
    entity.addComponent<hm::RigidBody>(body);
    entity.addComponent<hm::Mesh>(sphere_mesh);

    auto material = entity.addComponent<hm::Material>();

    material().diff_type = hm::Material::Other;
    material().diff_color = vec3(1.0f);

    material().metalness = random_floats(random_generator) * 0.8f;
    material().roughness = random_floats(random_generator) * 0.8f;
    material().ior = vec3(1.47f);

    world.addRigidBody(body);

    auto vis = entity.addComponent<hm::Visibility>();

    ek::VisibilityMesh vis_mesh;
    vis_mesh.model = transform().t.matrix();
    vis_mesh.aabb = AABB(vec3(-1.0f), vec3(1.0f));
    vis_mesh.num_verts = (u32)sphere_verts.size();
    vis_mesh.num_inds = (u32)sphere_inds.size();

    vis_mesh.verts = StridePtr<const vec3>(sphere_verts.data(), sizeof(mesh::PNVertex));
    vis_mesh.inds = sphere_inds.data();

    vis().vis.addMesh(std::move(vis_mesh));

    num_spheres++;

    return entity;
  };

  unsigned num_light_spheres = 0;
  auto create_light_sphere = [&](vec3 origin, vec3 color)
  {
    const float LightSphereRadius = random_floats(random_generator)*10.0f;

    origin += vec3(LightSphereRadius);

    auto name = util::fmt("light_sphere%u", num_light_spheres);
    auto light_name = util::fmt("lights%u", num_light_spheres);
    num_light_spheres++;

    auto sphere_entity = hm::entities().createGameObject(name, scene);
    auto light_entity = hm::entities().createGameObject(light_name, sphere_entity);

    const float AABBDiagonal = sqrtf(3)*LightSphereRadius;
    auto transform = sphere_entity.addComponent<hm::Transform>(
      xform::Transform(origin, quat::identity(), vec3(LightSphereRadius)),
      AABB(
        origin - vec3(AABBDiagonal),
        origin + vec3(AABBDiagonal)
      )
    );
    sphere_entity.addComponent<hm::Mesh>(sphere_mesh);

    auto material = sphere_entity.addComponent<hm::Material>();

    material().diff_type = (hm::Material::DiffuseType)(hm::Material::Other | 1);
    material().diff_color = color * 100.0f;

    light_entity.addComponent<hm::Transform>(xform::Transform());
    auto light = light_entity.addComponent<hm::Light>();

    light().type = hm::Light::Sphere;
    light().color = color;
    light().radius = 100.0f;
    light().sphere.radius = LightSphereRadius;

    auto vis = sphere_entity.addComponent<hm::Visibility>();

    ek::VisibilityMesh vis_mesh;
    vis_mesh.model = transform().t.matrix();
    vis_mesh.aabb = AABB(transform().aabb.min - origin, transform().aabb.max - origin);
    vis_mesh.num_verts = (u32)sphere_verts.size();
    vis_mesh.num_inds = (u32)sphere_inds.size();

    vis_mesh.verts = StridePtr<const vec3>(sphere_verts.data(), sizeof(mesh::PNVertex));
    vis_mesh.inds = sphere_inds.data();

    vis().vis.addMesh(std::move(vis_mesh));
    vis().vis.flags(ek::VisibilityObject::Occluder);

    return sphere_entity;
  };

  auto line_mesh = mesh::Mesh()
    .withIndexedArray(line_arr_id)
    .withNum((u32)line_inds.size());
  unsigned num_light_lines = 0;
  auto create_light_line = [&](vec3 center, float length, vec3 color)
  {
    auto name = util::fmt("light_line%u", num_light_lines);
    auto light_name = util::fmt("lightl%u", num_light_lines);
    num_light_lines++;

    auto line_entity = hm::entities().createGameObject(name, scene);
    auto light_entity = hm::entities().createGameObject(light_name, line_entity);

    auto scale = vec3(length, 1.0f, 1.0f);
    auto transform = line_entity.addComponent<hm::Transform>(
      xform::Transform(center, quat::identity(), scale),
      AABB(center - scale, center + scale)
    );
    line_entity.addComponent<hm::Mesh>(line_mesh);

    auto material = line_entity.addComponent<hm::Material>();

    material().diff_type = (hm::Material::DiffuseType)(hm::Material::Other | 1);
    material().diff_color = color * 100.0f;

    light_entity.addComponent<hm::Transform>(xform::Transform());
    auto light = light_entity.addComponent<hm::Light>();

    light().type = hm::Light::Line;
    light().color = color;
    light().radius = 1.0f;
    light().line.tangent = vec3::up();
    light().line.length = length;

    auto vis = line_entity.addComponent<hm::Visibility>();

    ek::VisibilityMesh vis_mesh;
    vis_mesh.model = transform().t.matrix();
    vis_mesh.aabb = AABB(-scale*0.5f, scale*0.5f);
    vis_mesh.num_verts = (u32)line_vtxs.size();
    vis_mesh.num_inds = (u32)line_inds.size();

    vis_mesh.verts = StridePtr<const vec3>(line_vtxs.data(), sizeof(mesh::PVertex));
    vis_mesh.inds = line_inds.data();

    vis().vis.addMesh(std::move(vis_mesh));
    vis().vis.flags(ek::VisibilityObject::Occluder);

    return line_entity;
  };

  auto create_model = [&](const mesh::Mesh& mesh, const mesh::ObjMesh& obj,
    const mesh::ObjMesh& hull, const vec3 *hull_verts,
    const std::string& name = "")
  {
    vec3 origin = { 0.0f, 4.0f, -50.0f };
    vec3 scale(1.0f);

    AABB aabb = obj.aabb().scale(scale);
    aabb.min += origin; aabb.max += origin;

    vec3 extents = aabb.max - aabb.min;

    uint idx = 0;
    const auto& faces = hull.faces();
    auto model_shape = bt::shapes().convexHull([&](vec3 &dst) -> bool {
      if(idx >= faces.size()*3) return false;

      dst = hull_verts[faces[idx/3][idx%3].v];
      idx++;

      return true;
    });
    auto body = bt::RigidBody::create(model_shape, origin, 1.0f);

    auto entity = hm::entities().createGameObject(
      !name.empty() ? name : obj.name(),
      scene);

    entity.addComponent<hm::Transform>(
      xform::Transform(origin, quat::identity(), scale),
      aabb
    );
    entity.addComponent<hm::RigidBody>(body);
    entity.addComponent<hm::Mesh>(mesh);

    //world.addRigidBody(body);

    auto material = entity.addComponent<hm::Material>();

    material().diff_type = hm::Material::DiffuseTexture;
    material().diff_tex.id = tex_id;
    material().diff_tex.sampler_id = floor_sampler_id;
    //material().diff_type = hm::Material::DiffuseConstant;
    //material().diff_color = { 0.53f, 0.8f, 0.94f };

    material().metalness = 0.0f;
    material().roughness = 0.7f;
    material().ior = vec3(14.7f);

    auto vis = entity.addComponent<hm::Visibility>();

    vis().vis.flags(ek::VisibilityObject::Occluder);

    return entity;
  };

  auto create_lights = [&]()
  {
    //create_light_sphere({ 0.0f, 6.0f, 0.0f }, vec3(10.0f));
    create_light_sphere({ -10.0f, 6.0f, -10.0f }, vec3(10.0f, 10.0f, 0.0f));
    create_light_sphere({ 20.0f, 6.0f, 0.0f }, vec3(0.0f, 10.0f, 10.0f));

    create_light_line({ 0.0f, 5.0f, 2.0f }, 20.0f, vec3(1.0f));
  };

  auto floor = create_floor();
  hm::Entity bunny;

  auto ui_paint_job = sched::create_job([&]() -> gx::CommandBuffer {

    // Proof-of-concept for generating gx::CommandBuffers
    //   concurrently - the idea is that although OpenGL
    //   is single-threaded in nature, we can still upload
    //   all the buffers (which is one of the few things the 
    //   driver doesn't serialize accross threads) and record
    //   all the draw commands on a separate thread (which 
    //   MUST be MORE expensive or AT LEAST as expensive as
    //   the GL calls themselves otherwise the overhead of
    //   marshalling them into the gx::CommandBuffer makes
    //   this slower than doing it all on a single thread)
    //   and delegate their execution to the main thread,
    //   which (in theory) results in increased performace
    return iface.paint();
  });

  auto physics_step_job = sched::create_job([&](float step_dt) -> Unit {
    world.step(step_dt);

    return {};
  });

  auto transforms_extract_job = sched::create_job([&]() -> Unit {
    hm::components().requireUnlocked();

    // Update Transforms
    hm::components().foreach([&](hmRef<hm::RigidBody> rb) {
      if(rb().rb.isStatic()) return;

      auto entity = rb().entity();
      auto transform = entity.component<hm::Transform>();

      transform() = rb().rb.worldTransform();
      transform().aabb = rb().rb.aabb();
    });

    hm::components().endRequireUnlocked();

    return {};
  });

  double transforms_extract_dt = 0.0;

  bool use_ao = true;

  win32::DeltaTimer time;
  time.reset();

  size_t gpu_frametime = 0;

  create_lights();

  std::vector<std::vector<vec3>> verts_vec;
  std::vector<std::vector<u16>> inds_vec;

  while(window.processMessages()) {
    using hm::entities;
    using hm::components;

    auto std_stream = win32::StdStream::gets();

    win32::Timers::tick();
    if(std_stream.size()) console.print(std_stream);

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
          components().foreach([&](hmRef<hm::RigidBody> rb) {
            world.removeRigidBody(rb().rb);
          });

          scene.destroy();
          scene = entities().createGameObject("Scene");

          scene.addComponent<hm::Transform>(xform::Transform());

          floor = create_floor();
          create_lights();
        } else if(kb->keyDown('Q')) {
          window.quit();
        } else if(kb->keyDown('A')) {
          use_ao = !use_ao;
        } else if(kb->keyDown('D')) {
          create_sphere();
        } else if(kb->keyDown('W')) {
          //pipeline.isEnabled(gx::Pipeline::Wireframe) ? pipeline.filledPolys() : pipeline.wireframe();
        } else if(kb->keyDown('`')) {
          console.toggle();
        } else if(kb->keyDown('F')) {
          pbo.downloadFramebuffer(fb_composite, FramebufferSize.x, FramebufferSize.y,
            gx::rgb, gx::u8);

          //pbo.downloadTexture(tex, 1, gx::rgb, gx::u8);

          auto pbo_view = pbo.map(gx::Buffer::Read, 0, PboSize);

          win32::File screenshot("screenshot.bin", win32::File::Write, win32::File::CreateAlways);
          screenshot.write(pbo_view.get(), (win32::File::Size)PboSize);
        } else if(kb->key == Keyboard::Up) {
          zoom = clamp(zoom+0.05f, 0.01f, INFINITY);
        } else if(kb->key == Keyboard::Down) {
          zoom = clamp(zoom-0.05f, 0.01f, INFINITY);
        }
      } else if(auto mouse = input->get<win32::Mouse>()) {
        using win32::Mouse;

        cursor.visible(!mouse->buttons);
        if(mouse->buttonDown(Mouse::Left)) iface.keyboard(nullptr);

        if(mouse->buttons & Mouse::Left) {
          mat4 d_mtx = mat4::identity()
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

          zoom_mtx = mat4::identity();
        }

        if(mouse->buttonDown(Mouse::Left)) {
          nudge_timer.reset();
        } else if(mouse->buttonUp(Mouse::Left)) {
          nudge_force = nudge_timer.elapsedSecondsf();
          nudge_timer.stop();
        }
      }
    }

    gx::Query query(gx::Query::TimeElapsed);
    auto query_scope = query.begin();
    query.label("qFrametime");

    // All the input has been processed - schedule a Ui paint
    auto ui_paint_job_id = worker_pool.scheduleJob(ui_paint_job.withParams());

    mat4 model = mat4::identity();

    auto persp = xform::perspective(70.0f,
      16.0f/9.0f, 30.0f, 1e6);

    auto view = xform::look_at(eye.xyz(), pos, vec3{ 0, 1, 0 });

    frustum3 frustum(view, persp, false);

    auto texmatrix = xform::Transform()
      .scale(3.0f)
      .matrix()
      ;

    vec4 mouse_ray = xform::unproject({ cursor.pos(), 0.5f }, persp*view, FramebufferSize);
    vec3 mouse_ray_direction = vec4::direction(eye, mouse_ray).xyz();

    ivec2 shadowmap_size = { 1024, 1024 };

    auto timef = time.elapsedSecondsf() * 0.1;
    vec3 shadow_pos = vec3(50.0f)*vec3(cos(timef), 1.0f, sin(timef));

    auto shadow_view_mtx = xform::look_at(shadow_pos, vec3::zero(), vec3::up());
    auto shadow_proj = xform::ortho(60, -60, -60, 60, -10, 150);

    auto shadow_view = ek::RenderView(ek::RenderView::ShadowView)
      .depthOnlyRender()
      .viewport(shadowmap_size)
      .view(shadow_view_mtx)
      .projection(shadow_proj);

    auto render_view = ek::RenderView(ek::RenderView::CameraView)
      .addInput(&shadow_view)
      .forwardRender()
      .viewport(FramebufferSize)
      .view(view)
      .projection(persp);

    bt::RigidBody picked_body;
    hm::Entity picked_entity = hm::Entity::Invalid;
    bool draw_nudge_line = false;
    vec3 hit_normal;
    if(mouse_ray.w != 0.0f) {
      auto hit = world.rayTestClosest(bt::Ray::from_direction(eye.xyz(), mouse_ray_direction));
      if(hit) {
        picked_body = hit.rigidBody();
        picked_entity = hit.rigidBody().user<hm::Entity>();
        hit_normal  = hit.normal();

        if(!picked_body.isStatic()) draw_nudge_line = true;
      }
    }

    if(nudge_force > 0.0f && picked_body) {
      auto center_of_mass = picked_body.centerOfMass();
      float force_factor = 1.0f + pow(nudge_force, 3.0f)*10.0f;

      picked_body
        .activate()
        .applyImpulse(-hit_normal*force_factor, center_of_mass);
    }

    // Kick off the physics simulation - DO NOT touch any physics-related
    //   structures before waiting for completion
    float step_dt = step_timer.elapsedSecondsf();
    auto physics_step_job_id = worker_pool.scheduleJob(
      physics_step_job.withParams(step_dt)
    );

    step_timer.reset();

    auto extract_for_view_job = ek::renderer().extractForView(scene, render_view);
    auto extract_for_view_job_id = worker_pool.scheduleJob(extract_for_view_job.get());

    auto extract_for_shadows_job = ek::renderer().extractForView(scene, shadow_view);
    auto extract_for_shadows_job_id = worker_pool.scheduleJob(extract_for_shadows_job.get());

    if(model_load_job->done() && model_load_job_id != sched::WorkerPool::InvalidJob) {
      worker_pool.waitJob(model_load_job_id);
      model_load_job_id = sched::WorkerPool::InvalidJob;

      ((mesh::MeshLoader&)obj_hull_loader).load();
      auto hull_verts = obj_hull_loader.vertices().data();

      vec3 origin = { 0.0f, 4.0f, -50.0f };

      hm::components().requireUnlocked();

      verts_vec.reserve(obj_loader.numMeshes());
      inds_vec.reserve(obj_loader.numMeshes());

      for(uint i = 0; i < obj_loader.numMeshes(); i++) {
        auto& mesh = obj_loader.mesh(i);
        auto& hull = obj_hull_loader.mesh(0);

        auto num_inds = mesh.faces().size() * 3;
        auto bunny_mesh = mesh::Mesh()
          .withNormals()
          .withTexCoords(1)
          .withIndexedArray(bunny_arr_id)
          .withNum((u32)num_inds)
          .withOffset((u32)mesh.offset());

        bunny = create_model(bunny_mesh, mesh, hull, hull_verts);

        auto& verts = verts_vec.emplace_back();
        verts.reserve(mesh.faces().size());
        auto& inds = inds_vec.emplace_back();
        inds.reserve(mesh.faces().size());
        for(auto& face : mesh.faces()) {
          auto idx = verts.size();

          verts.push_back(obj_loader.vertices().at(face[0].v));
          inds.push_back((u16)idx);
          verts.push_back(obj_loader.vertices().at(face[1].v));
          inds.push_back((u16)idx + 1);
          verts.push_back(obj_loader.vertices().at(face[2].v));
          inds.push_back((u16)idx + 2);
        }

        auto aabb = mesh.aabb();
        auto vis_object = bunny.component<hm::Visibility>().get().visObject();

        vis_object->addMesh(ek::VisibilityMesh::from_vectors(
          xform::translate(origin), aabb,
          verts_vec[i], inds_vec[i]
        ));
      }

      hm::components().endRequireUnlocked();
    }

    if(draw_nudge_line && 0) {
      float force_factor = 1.0f + pow(nudge_timer.elapsedSecondsf(), 3.0f);

      quat q = quat::rotation_between(vec3::up(), hit_normal);
    }

    std::vector<hm::Entity> dead_entities;

    worker_pool.waitJob(extract_for_shadows_job_id);
    auto& shadow_objects = extract_for_shadows_job->result();
    auto shadow_render_job = shadow_view.render();
    auto shadow_render_job_id = worker_pool.scheduleJob(shadow_render_job->withParams(&shadow_objects));

    worker_pool.waitJob(extract_for_view_job_id);
    auto& render_objects = extract_for_view_job->result();
    auto render_job = render_view.render();
    auto render_job_id = worker_pool.scheduleJob(render_job->withParams(&render_objects));

    worker_pool.waitJob(shadow_render_job_id);
    worker_pool.waitJob(render_job_id);

    shadow_render_job->result().execute();
    render_job->result().execute();

    auto occlusion_buf = render_view.visibility().occlusionBuf().detiledFramebuffer();

    occlusion_tex.upload(occlusion_buf.get(), 0,
      0, 0, ek::OcclusionBuffer::Size.x, ek::OcclusionBuffer::Size.y, gx::r, gx::f32);
    occlusion_tex.swizzle(gx::Red, gx::Red, gx::Red, gx::One);


    skybox_uniforms = { view, persp };
    skybox_program.use()
      .uniformFloat(U.skybox.uExposure, exp_slider.value())
      ;

   // cmd_skybox
   //   .execute();

    if(picked_entity && picked_entity.alive()) {
      if(picked_entity.gameObject().parent() == scene) {
        auto transform = picked_entity.component<hm::Transform>();

        small_face.draw(util::fmt("picked(0x%.8x) at: %s",
          picked_entity.id(), math::to_str(transform.get().t.translation())),
          { 30.0f, 100.0f+small_face.height()*2.8f }, { 1.0f, 0.0f, 0.0f });
      }
    }

    // Draw entity names, ids and origins in columns
    float entity_str_width = 300.0f;
    float entity_str_origin_y = 170.0f;
    float y = entity_str_origin_y;
    float x = 30.0f;
    scene.gameObject().foreachChild([&](hm::Entity entity) {
      return;
      if(!entity.hasComponent<hm::Transform>()) return;

      if(y > FramebufferSize.y-small_face.height()) {
        x += entity_str_width;
        y = entity_str_origin_y;
      }

      if(x+entity_str_width > FramebufferSize.x) return; // Cull invisible text

      auto transform = entity.component<hm::Transform>();
      small_face.draw(util::fmt("%.20s(0x%.8x) at: %s",
        entity.gameObject().name(), entity.id(), math::to_str(transform().t.translation())),
        { x, y }, { 1.0f, 1.0f, 1.0f });
      y += small_face.height();
    });

    worker_pool.waitJob(physics_step_job_id);

    auto transforms_extract_job_id = worker_pool.scheduleJob(transforms_extract_job.withParams());

    float fps = 1.0f / step_dt;

    constexpr float smoothing = 0.95f;
    old_fps = fps;
    fps = old_fps*smoothing + fps*(1.0f-smoothing);

    face.draw(util::fmt("FPS: %.2f", fps),
      vec2{ 30.0f, 70.0f }, { 0.8f, 0.0f, 0.0f });

    // Wait for Ui painting to finish
    worker_pool.waitJob(ui_paint_job_id);

    // Display the statistics
    stats.caption(util::fmt(
      "CPU Frametime: %.3lfms\n"
      "GPU Frametime: %.3lfms\n"
      "Main camera drawcalls: %zu\n"
      "Physics update: %.3lfms\n"
      "Ui painting: %.3lfms\n"
      "Transform extraction: %.3lfms",
      step_dt*1000.0,
      gpu_frametime*1e-6,
      render_view.numEmmittedDrawcalls(),
      physics_step_job.dbg_ElapsedTime()*1000.0,
      ui_paint_job.dbg_ElapsedTime()*1000.0,
      transforms_extract_dt*1000.0)
    );

#if 0
    float proj_scale = (float)FramebufferSize.y / (tanf(70.0f * 0.5f) * 2.0);

    vec4 proj_info ={
      2.0f / persp(0, 0),
      2.0f / persp(1, 1),
      -(1.0f - persp(2, 0)) / persp(0, 0),
      -(1.0f + persp(2, 1)) / persp(1, 1)
    };

    float ao_radius = ao_r_slider.value();
    ao_radius *= 0.5 * proj_scale;

    float ao_bias = ao_bias_slider.value();

    float light_x = fmod(time.elapsedSecondsf()*1.0f*PIf, 2.0f*PIf);
    vec4 light_dir = vec4(cosf(light_x), sinf(light_x), 0.0f, 1.0f);

    light_dir = view*light_dir;
    light_dir = light_dir.normalize();

    ao_pass.begin(pool);
    ao_program.use()
      .uniformMatrix4x4(U.ao.uInverseView, view.inverse())
      .uniformMatrix4x4(U.ao.uProjection, persp)
      .uniformVector4(U.ao.uProjInfo, proj_info)
      .uniformFloat(U.ao.uRadius, ao_radius)
      .uniformFloat(U.ao.uRadius2, ao_radius*ao_radius)
      .uniformFloat(U.ao.uNegInvRadius2, -1.0 / (ao_radius*ao_radius))
      .uniformFloat(U.ao.uBias, ao_bias)
      .uniformFloat(U.ao.uBiasBoostFactor, 1.0 / (1.0 - ao_bias))
      .uniformFloat(U.ao.uNear, 50.0f)
      .uniformVector3(U.ao.uLightPos, light_dir.xyz())
      .draw(gx::TriangleFan, fullscreen_quad_arr, fullscreen_quad.size())
      ;

#endif
    ui_paint_job.result().execute();

    cursor.paint();
    worker_pool.waitJob(transforms_extract_job_id);
    transforms_extract_dt = transforms_extract_job.dbg_ElapsedTime();

    hm::components().requireUnlocked();

    // Kill off dead_entites
    for(auto& e : dead_entities) {
      world.removeRigidBody(e.component<hm::RigidBody>().get().rb);
      e.destroy();
    }

    hm::components().endRequireUnlocked();

    const auto& render_view_rt = render_view.presentRenderTarget();
    auto& render_view_fb = pool.get<gx::Framebuffer>(render_view_rt.framebufferId());

    const auto& shadow_view_rt = shadow_view.presentRenderTarget();
    auto& shadow_view_fb = pool.get<gx::Framebuffer>(shadow_view_rt.framebufferId());

    const uint UiTexImageUnit = 4;
    const uint SceneTexImageUnit = 5;

    // Resolve the main Framebuffer and composite the Ui on top of it
    composite_pass.textures({
      { UiTexImageUnit, { iface.framebufferTextureId(),
                          resolve_sampler_id } },
      { SceneTexImageUnit, { render_view_rt.textureId(ek::RenderTarget::Accumulation),
                             resolve_sampler_id } },
      });

    composite_pass.begin(pool);

    composite_program.use()
      .uniformSampler(U.composite.uUi, UiTexImageUnit)
      .uniformSampler(U.composite.uScene, SceneTexImageUnit)
      .draw(gx::TriangleFan, fullscreen_quad_arr, fullscreen_quad.size());

    query_scope.end();

    fb_composite.blitToWindow(
      ivec4{ 0, 0, FramebufferSize.x, FramebufferSize.y },
      ivec4{ 0, 0, (int)WindowSize.x, (int)WindowSize.y },
      gx::Framebuffer::ColorBit, gx::Sampler::Linear);

    shadow_view_fb.blitToWindow(
      ivec4{ 0, 0, shadowmap_size.x, shadowmap_size.y },
      ivec4{ 0, 0, 256, 256 },
      gx::Framebuffer::ColorBit, gx::Sampler::Linear);

    static constexpr auto OccSz = ek::OcclusionBuffer::Size;
    occlusion_fb.blitToWindow(
      ivec4{ 0, OccSz.y, OccSz.x, 0 },
      ivec4{ (int)WindowSize.x - OccSz.x, OccSz.y, (int)WindowSize.x, 0 },
      gx::Framebuffer::ColorBit, gx::Sampler::Linear);

    window.swapBuffers();

    // The result should be ready here...
    gpu_frametime = query.resultsz();
  }

  worker_pool.killWorkers();

  window.destroy();

  ek::finalize();
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
