#include <common.h>

// Hamil/Generated  <-  Eugene
#include <resources.h>
#include <components.h>

//   util
#include <util/unit.h>
#include <util/polystorage.h>
#include <util/fixedbitvector.h>
#include <util/abstracttuple.h>

//   os
#include <os/os.h>
#include <os/cpuid.h>
#include <os/path.h>
#include <os/time.h>
#include <os/thread.h>
#include <os/window.h>
#include <os/input.h>
#include <os/file.h>

//   sysv
#include <sysv/x11.h>
#include <sysv/window.h>
#include <sysv/glcontext.h>

//   cli
#include <cli/cli.h>

//   sched
#include <sched/job.h>
#include <sched/pool.h>

//   gx
#include <gx/gx.h>
#include <gx/info.h>
#include <gx/pipeline.h>
#include <gx/resourcepool.h>
#include <gx/commandbuffer.h>
#include <gx/framebuffer.h>

//   ft
#include <ft/font.h>

//   res
#include <res/handle.h>
#include <res/image.h>
#include <res/res.h>
#include <res/texture.h>

//   ui
#include <ui/ui.h>
#include <ui/uicommon.h>
#include <ui/style.h>
#include <ui/cursor.h>
#include <ui/frame.h>
#include <ui/layout.h>
#include <ui/window.h>
#include <ui/label.h>
#include <ui/console.h>

//   ek
#include <ek/euklid.h>
#include <ek/renderer.h>

// hm
#include <hm/hamil.h>
#include <hm/entity.h>
#include <hm/entityman.h>
#include <hm/component.h>
#include <hm/componentmeta.h>
#include <hm/prototype.h>
#include <hm/prototypecache.h>
#include <hm/prototypechunk.h>
#include <hm/cachedprototype.h>
#include <hm/chunkhandle.h>
#include <hm/chunkman.h>
#include <hm/world.h>
#include <hm/queryparams.h>
#include <hm/entityquery.h>
#include <hm/components/all.h>

//   libc++/STL
#include <type_traits>
#include <numeric>
#include <vector>
#include <array>
#include <random>

//   libc
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cstdio>

int main(int argc, char *argv[])
{
  os::init();

  if(argc > 1) {
    if(auto exit_code = cli::args(argc, argv)) exit(exit_code);
  }

#if 1
  hm::init();

  auto& world = hm::world()
    .createEmpty();

  auto my_proto_desc = hm::EntityPrototype({
      hm::ComponentProto::GameObject,
      hm::ComponentProto::Transform,
      hm::ComponentProto::Light,
  });
  auto my_proto2_desc = hm::EntityPrototype({
      hm::ComponentProto::GameObject,
      hm::ComponentProto::Transform,
      hm::ComponentProto::Visibility,
      hm::ComponentProto::Hull,
      hm::ComponentProto::Mesh,
  });

  auto my_proto = world.entities().prototype(my_proto_desc);
  auto my_proto2 = world.entities().prototype(my_proto2_desc);


  for(auto i = 0; i < 250; i++) world.entities().createEntity(my_proto);
  for(auto i = 0; i < 25; i++) world.entities().createEntity(my_proto2);


  /*
  world.entities().prototypeCache()
      ->foreachCachedProto([](hm::EntityPrototypeCache::PrototypeDesc p) {
          printf("PrototypeDesc -> EntityPrototype<0x%u> (%u chunks/%u entities)\n",
              p.cache_id, (u32)p.numChunks(), (u32)p.numEntities());
          p.prototype.dbg_PrintComponents();
          printf("\t[[     -------       ]]\n\n");
      })
      */
  world.entities().prototypeCache()
      .dbg_PrintPrototypeCacheStats();

//  auto e = world.entities().createEntity(my_proto2);
//  auto e2 = world.entities().createEntity(my_proto2);
  auto query_params = hm::EntityQueryParams::create_dbg();

  hm::EntityQuery eq(
      world.entities()
          .createEntityQuery(query_params)
          .injectCreationParams(query_params));

  eq.collectEntities()
    .dbg_PrintCollectedChunks();

  auto& chunk_man = *world.entities()
      .chunkManager();

  auto print_chunk = [&](hm::CachedPrototype &proto, size_t chunk_idx)
  {
    auto c = world.entities()
                 .prototypeBoundChunk(proto, chunk_idx);

    printf(
        "PrototypeChunk<%s> (%zu entities) ->\n"
        "\tGameObject:    %p -> %p\n"
        "\tHull:          %p -> %p\n",
        util::to_str(proto.prototype().components()).data(),
        proto.chunkByIndex(chunk_idx).numEntities(),
        c.componentDataArrayBegin<hm::GameObject>(),
        c.componentDataArrayEnd<hm::GameObject>(),
        c.componentDataArrayBegin<hm::Transform>(),
        c.componentDataArrayEnd<hm::Transform>());
  };

  world.entities()
    .prototypeBoundChunk(my_proto, 0)
    .componentDataForeach<hm::Transform>(
      [&](hm::Transform& t, u32 entity_idx) {
        auto& chunk = world.entities()
            .prototypeBoundChunk(my_proto, 0);
        auto chunk_ptr = chunk.data();

        world.entities().prototype().

        printf(
        );
      }
    );

#endif

#if 0
  auto probe_cached_proto = [&](const hm::EntityPrototype& proto) {
    //printf("hm::CachedPrototype(0x%.8x):\n        ", proto.hash());

    auto cached_proto = proto_cache.probe(proto);
    if(!cached_proto.has_value()) {
      puts("    hm::EntityPrototypeCache::probe() -> std::nullopt!\n\n");
      return;
    }

    printf(
        "    .numChunks=%zu .numEntities=%zu\n\n",
        cached_proto->numChunks(), cached_proto->numEntities()
    );
  };

  auto my_proto = hm::EntityPrototype({
      hm::ComponentProto::GameObject,
      hm::ComponentProto::Transform,
      hm::ComponentProto::Light,
  });

  printf("EntityPrototype(%zu Components):\n", my_proto.numProtoComponents());
  my_proto.dbg_PrintComponents();

  puts("");

  auto my_proto2 = hm::EntityPrototype({
      hm::ComponentProto::GameObject,
      hm::ComponentProto::Transform,
      hm::ComponentProto::Visibility,
      hm::ComponentProto::Hull,
      hm::ComponentProto::Mesh,
  });

  printf("EntityPrototype(%zu Components):\n", my_proto2.numProtoComponents());
  my_proto2.dbg_PrintComponents();
#endif


#if 0
  auto rb_metaclass = hm::metaclass_from_protoid(hm::ComponentProto::RigidBody);

  const auto rb_static = rb_metaclass->staticData();

  printf(
      "hm::metaclass_from_protoid(hm::RigidBody).static:\n"
      "      .proto_id  = %s (%u)\n"
      "      .data_size = %zu\n"
      "      .flags     = 0x%.8x\n",
      hm::protoid_to_str(rb_static.protoid).data(), rb_static.protoid,
      rb_static.data_size,
      rb_static.flags
  );

  using SomeChunkB = hm::PrototypeChunk<
    hm::GameObject, hm::Transform, hm::RigidBody
  >;

  SomeChunkB chunk_b;

  auto go_b = chunk_b.storage<hm::GameObject>();
  auto t_b  = chunk_b.storage<hm::Transform>();
  auto rb_b = chunk_b.storage<hm::RigidBody>();

  printf(
      "&PrototypeChunk<GameObject, Transform, RigidBody>: %p\n"
      "        sizeof=%zu NumComponentsPerType=%zu\n"
      "    .storage<GameObject>: %p (offset=%zu)\n"
      "    .storage<Transform>:  %p (offset=%zu)\n"
      "    .storage<RigidBody>:  %p (offset=%zu)\n"
      "\n",
    &chunk_b.m_storage,
    sizeof(SomeChunkB), SomeChunkB::NumComponentsPerType,
    go_b, (u8 *)go_b-(u8 *)&chunk_b.m_storage,
    t_b, (u8 *)t_b-(u8 *)&chunk_b.m_storage,
    rb_b, (u8 *)rb_b-(u8 *)&chunk_b.m_storage
  );


  return 0;
#endif

#if 0
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
        .background(ui::blue().darkenf(0.8).opacity(0.4))
        .geometry(ui::Geometry(vec2(200.0f, 100.0f), vec2(150.0f, 150.0f)))
        .gravity(ui::Frame::Center))
    .frame(ui::create<ui::WindowFrame>(iface)
        .title("Pineapple")
        .content(ui::create<ui::LabelFrame>(iface)
          .drawable(pineapple))
        .background(ui::green().lightenf(0.0))
        .geometry(ui::Geometry(vec2(500.0f, 100.0f), vec2(512.0f, 512.0f))))
    .frame(ui::create<ui::ConsoleFrame>(iface, "g_console"))
    ;

  auto& ui_console = *iface.getFrameByName<ui::ConsoleFrame>("g_console");

  ft::Font face(ft::FontFamily("dejavu-serif"), 20);

  auto pipeline = gx::Pipeline(&pool)
      .add<gx::Pipeline::Viewport>(0, 0, 1280, 720)
      .add<gx::Pipeline::Scissor>([](auto& sc) {
          sc.no_test();
      })
      .add<gx::Pipeline::ClearColor>(vec4(0.2f, 0.4f, 0.2, 1.0f))
      .add<gx::Pipeline::Blend>([](auto& b) {
          b.alpha_blend();
      })
  ;

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
