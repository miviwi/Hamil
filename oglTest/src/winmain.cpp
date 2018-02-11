#include <util/format.h>

#include <math/geometry.h>
#include <math/transform.h>

#include <win32/win32.h>
#include <win32/window.h>
#include <win32/time.h>

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

#include <game/game.h>

#include <glang/glang.h>
#include <glang/heap.h>
#include <glang/import.h>

#include <vector>
#include <array>
#include <utility>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  win32::init();

  using win32::Window;
  Window window(1280, 720);

  constexpr ivec2 FB_DIMS{ 1280, 720 };

  gx::init();
  ft::init();
  ui::init();
  game::init();

  auto heap = glang::WinHeap();
  auto *vm = new glang::Vm(&heap);

  auto& om = vm->objMan();

  auto vm_messagebox = glang::CallableFn<StringObject, StringObject>(
    [](glang::ObjectManager& om, auto args) -> Object *
  {
    StringObject *title = std::get<0>(args),
      *text = std::get<1>(args);

    MessageBoxA(nullptr, text->get(), title->get(), MB_OK);

    return nullptr;
  });
  auto vm_messagebox_fn = om.fn(&vm_messagebox);
  vm->envSet("messagebox", vm_messagebox_fn);

  vm->eval("(def v [:a :b :c :d])");

  ft::Font face(ft::FontFamily("georgia"), 35);
  ft::Font small_face(ft::FontFamily("segoeui"), 12);

  ui::CursorDriver cursor(1280/2, 720/2);
  vec3 pos{ 0, 0, 0 };
  float pitch = 0, yaw = 0;
  float zoom = 1.0f, rot = 0.0f;

  bool constrained = true;

  int animate = -1;

  auto fmt = gx::VertexFormat()
    .attr(gx::f32, 3)
    .attr(gx::f32, 3);

  u32 tex_image[] = {
    0xFF000000, 0xFF000000, 0xFFFFFF00, 0xFFFFFF00,
    0xFF000000, 0xFF000000, 0xFFFFFF00, 0xFFFFFF00,
    0xFFFFFF00, 0xFFFFFF00, 0xFF000000, 0xFF000000,
    0xFFFFFF00, 0xFFFFFF00, 0xFF000000, 0xFF000000,
  };

  gx::Texture2D tex(gx::rgb);
  auto sampler = gx::Sampler()
    .param(gx::Sampler::WrapS, gx::Sampler::Repeat)
    .param(gx::Sampler::WrapT, gx::Sampler::Repeat)
    .param(gx::Sampler::MinFilter, gx::Sampler::Linear)
    .param(gx::Sampler::MagFilter, gx::Sampler::Linear);

  tex.init(tex_image, 0, 4, 4, gx::rgba, gx::u32_8888);

  gx::tex_unit(0, tex, sampler);

  int vram_size;
  glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &vram_size);

  char vram_size_buf[256];
  sprintf_s(vram_size_buf, "VRAM: %.2fGB", (float)vram_size/(1024.0f*1024.0f));
  auto vram_size_str = face.string(vram_size_buf);

  unsigned cp = 0;

  const char *vs_src = R"VTX(
uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormal;

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iNormal;

out VertexData {
  vec3 position;
  vec3 normal;
} vertex;

void main() {
  vertex.position = vec3(uModelView * vec4(iPosition, 1.0));
  vertex.normal = uNormal * iNormal;

  gl_Position = uProjection * vec4(vertex.position, 1.0);
}
)VTX";

  const char *phong_src = R"GLSL(
const float gamma = 2.2;

struct Light {
  vec3 position;
  vec3 color;
};

uniform LightBlock {
  Light lights[4];
  int num_lights;
};

vec3 phong(Light light, vec3 position, vec3 normal) {
  vec3 light_direction = normalize(light.position - position);
  float distance = length(light.position - position);

  float attenuation = 1.0 / (1 + 0.35*distance + 0.44*(distance*distance));

  float diffuse_strength = max(dot(normal, light_direction), 0.0);
  
  vec3 view_direction = normalize(-position);
  vec3 reflect_direction = reflect(-light_direction, normal);

  float specular_strength = pow(max(dot(view_direction, reflect_direction), 0.0), 32);
  
  vec3 ambient = 0.1 * light.color;
  vec3 diffuse = diffuse_strength * light.color;
  vec3 specular = 0.5 * specular_strength * light.color;

  return (ambient+diffuse+specular)*attenuation;
} 

vec3 toGammaSpace(vec3 color) {
  return pow(color, vec3(1.0/gamma));
}

vec4 toGammaSpace(vec4 color) {
  return vec4(pow(color.rgb, vec3(1.0/gamma)), color.a);
}

)GLSL";

  const char *fs_src = R"FG(
uniform vec4 uCol;

in VertexData {
  vec3 position;
  vec3 normal;
} fragment;

layout(location = 0) out vec3 color;

void main() {
  vec3 normal = normalize(fragment.normal);

  vec3 light = vec3(0, 0, 0);
  for(int i = 0; i < num_lights; i++) {
    light += phong(lights[i], fragment.position, normal);
  }

  if(uCol.a < 0.0) {
    color = toGammaSpace(light*uCol.rgb);
  } else {
    int index = int(uCol.a);
    color = lights[index].color;
  }
}
)FG";

  const char *tex_vs_src = R"VTX(
uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormal;
uniform mat4 uTexMatrix;

layout(location = 0) in vec2 iPosition;
layout(location = 1) in vec2 iTexCoord;

out VertexData {
  vec3 position;
  vec3 normal;
  vec2 tex_coord;
} vertex;

void main() {
  vertex.position = vec3(uModelView * vec4(iPosition, 0.0, 1.0));
  vertex.normal = uNormal * vec3(0.0, 0.0, -1.0);
  vertex.tex_coord = (uTexMatrix * vec4(iTexCoord, 0.0f, 1.0f)).st;

  gl_Position = uProjection * vec4(vertex.position, 1.0f);
}
)VTX";

  const char *tex_fs_src = R"FG(
uniform sampler2D uTex;

in VertexData {
  vec3 position;
  vec3 normal;
  vec2 tex_coord;
} fragment;

layout(location = 0) out vec3 color;

void main() {
  vec3 normal = normalize(fragment.normal); 
  vec4 diffuse = texture(uTex, fragment.tex_coord);

  vec3 light = vec3(0, 0, 0);
  for(int i = 0; i < num_lights; i++) {
    light += phong(lights[i], fragment.position, normal);
  }

  color = toGammaSpace(light*diffuse.rgb);
}
)FG";

  gx::Shader vtx_shader(gx::Shader::Vertex, vs_src);
  gx::Shader frag_shader(gx::Shader::Fragment, { phong_src, fs_src });

  gx::Shader tex_vtx_shader(gx::Shader::Vertex, tex_vs_src);
  gx::Shader tex_frag_shader(gx::Shader::Fragment, { phong_src, tex_fs_src });

  gx::Program program(vtx_shader, frag_shader);
  gx::Program tex_program(tex_vtx_shader, tex_frag_shader);

  program.getUniformsLocations(U::program);
  tex_program.getUniformsLocations(U::tex);

  program.label("program");
  tex_program.label("TEX_program");

  gx::Texture2D fb_tex(gx::rgb8);
  gx::Framebuffer fb;

  fb_tex.initMultisample(2, FB_DIMS.x, FB_DIMS.y);
  fb_tex.label("FB_tex");

  fb.use()
    .tex(fb_tex, 0, gx::Framebuffer::Color(0))
    .renderbuffer(FB_DIMS.x, FB_DIMS.y, gx::depth24, gx::Framebuffer::Depth);

  auto pipeline = gx::Pipeline()
    .viewport(0, 0, FB_DIMS.x, FB_DIMS.y)
    .depthTest(gx::Pipeline::LessEqual)
    .cull(gx::Pipeline::Back)
    .clear(vec4{ 0.1f, 0.1f, 0.1f, 1.0f }, 1.0f);

  float r = 1280.0f;
  float b = 720.0f;

  mat4 ortho = xform::ortho(0, 0, b, r, 0.1f, 100.0f);

  mat4 zoom_mtx = xform::identity();
  mat4 rot_mtx = xform::identity();

  window.captureMouse();

  int frames = 0;
  auto start_time = GetTickCount();

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
    { -1.0f, 1.0f },  { 0.0f,  10.0f },
    { -1.0f, -1.0f }, { 0.0f,  0.0f },
    { 1.0f, -1.0f },  { 10.0f, 0.0f },

    { 1.0f, -1.0f },  { 10.0f, 0.0f },
    { 1.0f, 1.0f },   { 10.0f, 10.0f },
    { -1.0f, 1.0f },  { 0.0f,  10.0f },

    { -1.0f, 1.0f },  { 0.0f,  10.0f },
    { 1.0f, -1.0f },  { 10.0f, 0.0f },
    { -1.0f, -1.0f }, { 0.0f,  0.0f },

    { 1.0f, -1.0f },  { 10.0f, 0.0f },
    { -1.0f, 1.0f },  { 0.0f,  10.0f },
    { 1.0f, 1.0f },   { 10.0f, 10.0f },

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

  auto floor_sampler = gx::Sampler()
    .param(gx::Sampler::MinFilter, gx::Sampler::Nearest)
    .param(gx::Sampler::MagFilter, gx::Sampler::Nearest)
    .param(gx::Sampler::WrapS, gx::Sampler::Repeat)
    .param(gx::Sampler::WrapT, gx::Sampler::Repeat)
    .param(gx::Sampler::Anisotropy, 16.0f);

  auto alpha = (byte)(255*0.95);
  //auto color_a = ui::Color{ 45, 45, 150, alpha },
  //  color_b = ui::Color{ 20, 20, 66, alpha };
  
  auto color_a = ui::Color{ 150, 150, 45, alpha },
    color_b = ui::Color{ 66, 66, 20, alpha };

  ui::Style style;
  style.font = ft::Font::Ptr(new ft::Font(ft::FontFamily("segoeui"), 12));

  style.bg.color[0] = style.bg.color[3] = color_b;
  style.bg.color[1] = style.bg.color[2] = color_a;

  style.border.color[0] = style.border.color[3] = ui::white();
  style.border.color[1] = style.border.color[2] = ui::transparent();

  style.button.color[0] = color_b; style.button.color[1] = color_b.lighten(10);
  style.button.radius = 3.0f;
  style.button.margin = 1;
  
  style.slider.color[0] = color_b; style.slider.color[1] = color_b.lighten(10);
  style.slider.width = 10.0f;

  style.combobox.color[0] = color_b; style.combobox.color[1] = color_b.lighten(10);
  style.combobox.radius = 3.0f;

  ui::Ui iface(ui::Geometry{ 0, 0, 1280, 720 }, style);

  auto& layout = ui::create<ui::RowLayoutFrame>(iface)
    .frame(ui::create<ui::ColumnLayoutFrame>(iface)
           .frame<ui::PushButtonFrame>(iface, "b")
           .frame<ui::PushButtonFrame>(iface, "c"))
    .frame(ui::create<ui::DropDownFrame>(iface, "light_no")
           .item({ "Light 1" })
           .item({ "Light 2" })
           .item({ "Light 3" })
           .selected(0))
    .frame(ui::create<ui::ColumnLayoutFrame>(iface)
           .frame(ui::create<ui::LabelFrame>(iface).caption("Light X:"))
           .frame<ui::HSliderFrame>(iface, "x"))
    .frame(ui::create<ui::ColumnLayoutFrame>(iface)
           .frame(ui::create<ui::LabelFrame>(iface).caption("Light Y:"))
           .frame<ui::HSliderFrame>(iface, "y"))
    .frame(ui::create<ui::ColumnLayoutFrame>(iface)
           .frame(ui::create<ui::LabelFrame>(iface).caption("Light Z:"))
           .frame<ui::HSliderFrame>(iface, "z"))
    .frame(ui::create<ui::ColumnLayoutFrame>(iface)
           .frame(ui::create<ui::LabelFrame>(iface).caption("Toggle texmatrix:"))
           .frame<ui::CheckBoxFrame>(iface, "e")
           .gravity(ui::Frame::Left))
    ;

  auto btn_b = iface.getFrameByName<ui::PushButtonFrame>("b"),
    btn_c = iface.getFrameByName<ui::PushButtonFrame>("c");

  btn_b->caption("Quit Application").onClick([&](auto target) {
    window.quit();
  });
  btn_c->caption("Call vm!").onClick([&](auto target) {
    auto obj = vm->eval("(messagebox \"VM MessageBox!\" (repr (2 v)))");
    om.deref(obj);
  });

  auto& light_no = *iface.getFrameByName<ui::DropDownFrame>("light_no");

  auto& slider_x = iface.getFrameByName<ui::HSliderFrame>("x")->range(-5.0f, 5.0f)
    .onChange([&](auto target) {
    auto light_id = light_no.selected();
    auto pos = light_position[light_id];

    light_position[light_id] = {
      (float)target->value(), pos.y, pos.z
    };
  });
  auto& slider_y = iface.getFrameByName<ui::HSliderFrame>("y")->range(0.0f, 12.0f)
    .onChange([&](auto target) {
    auto light_id = light_no.selected();
    auto pos = light_position[light_id];

    light_position[light_id] = {
      pos.x, (float)target->value(), pos.z
    };
  });
  auto& slider_z = iface.getFrameByName<ui::HSliderFrame>("z")->range(-8.0f, 4.0f)
    .onChange([&](auto target) {
    auto light_id = light_no.selected();
    auto pos = light_position[light_id];

    light_position[light_id] = {
      pos.x, pos.y, (float)target->value()
    };
  });
  auto& checkbox = iface.getFrameByName<ui::CheckBoxFrame>("e")->value(false);

  light_no.onChange([&](auto target) {
    auto light_id = target->selected();
    auto pos = light_position[light_id];

    slider_x.value(pos.x);
    slider_y.value(pos.y);
    slider_z.value(pos.z);
  });

  iface
    .frame(layout, { 30.0f, 500.0f })
    .frame<ui::Frame>(iface, ui::Geometry{ 1000.0f, 300.0f, 200.0f, 300.0f })
    ;

  auto fps_timer = win32::DeltaTimer();

  constexpr auto anim_time = 10000.0f;
  auto anim_timer = win32::LoopTimer().durationSeconds(2.5);

  while(window.processMessages()) {
    win32::Timers::tick();

    vec4 eye{ 0, 0, 60.0f/zoom, 1 };

    mat4 eye_mtx = xform::identity()
      *xform::translate(pos*2.0f)
      *xform::roty(yaw)
      *xform::rotx(-pitch)
      *xform::translate(-pos)
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
        }
      } else if(auto mouse = input->get<win32::Mouse>()) {
        using win32::Mouse;

        cursor.visible(!(mouse->buttons & (Mouse::Left|Mouse::Right)));

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

    gx::tex_unit(0, tex, sampler);

    fb.use();
    pipeline.use();

    gx::clear(gx::Framebuffer::ColorBit|gx::Framebuffer::DepthBit);

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

    auto persp = !ortho_projection ? xform::perspective(70, 16.0f/9.0f, 0.1f, 1000.0f) :
      xform::ortho(9.0f, -16.0f, -9.0f, 16.0f, 0.1f, 1000.0f)*xform::scale(zoom*2.0f);

    //yaw = lerp(0.0f, 2.0f*PIf, anim_factor);
    //pitch = sin(2.0f*PIf * anim_factor) * PIf/2.0f;

    //vec3 eye{ (float)pitch/1280.0f*150.0f, (float)yaw/720.0f*150.0f, 60.0f/zoom };
    auto view = xform::look_at(eye.xyz(), pos, vec3{ 0, 1, 0 });

    vec4 color;

    light_block.num_lights = 3;

    light_block.lights[0] = {
      view*light_position[0],
      vec3{ 1.0f, 0.5f, 1.0f }
    };
    light_block.lights[1] = {
      view*light_position[1],
      vec3{ 0.7f, 1.0f, 1.0f }
    };
    light_block.lights[2] = {
      view*light_position[2],
      vec3{ 1.0f, 1.0f, 1.0f }
    };

    light_ubo.upload(&light_block, 0, 1);

    auto drawcube = [&]()
    {
      mat4 modelview = view*model;
      program.use()
        .uniformMatrix4x4(U::program.uProjection, persp)
        .uniformMatrix4x4(U::program.uModelView, modelview)
        .uniformMatrix3x3(U::program.uNormal, modelview.inverse().transpose())
        .uniformVector4(U::program.uCol, color)
        .draw(gx::Triangles, arr, vtxs.size());
    };

    auto rot = xform::identity()
      //*xform::rotz(lerp(0.0, PI, anim_factor))*0.5f
      *xform::roty(lerp(0.0, PI, anim_timer.elapsedf()))
      //*xform::rotz(lerp(0.0, PI, anim_factor))
      ;

    color = { 1.0f, 1.0f, 1.0f, -1.0f };
    model = xform::identity()
      *xform::translate(0.0f, 3.0f, 0.0f)
      //*xform::rotx(lerp(0.0f, PIf, anim_factor))
      //*xform::roty(lerp(0.0f, PIf, anim_factor))
      *xform::scale(1.5f)
      //*xform::translate(0.0f, 0.0f, -30.0f)
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
      .scale(1.0f/(sin((float)win32::Timers::timef_s() * PI/2.0f) + 2.0f))
      .rotz(lerp(0.0f, PIf, anim_timer.elapsedf()))
      .translate(5.0f, 5.0f, 0.0f)
      .matrix()
      ;

    mat4 modelview = view*model;

    gx::tex_unit(0, tex, floor_sampler);
    tex_program.use()
      .uniformMatrix4x4(U::tex.uProjection, persp)
      .uniformMatrix4x4(U::tex.uModelView, modelview)
      .uniformMatrix3x3(U::tex.uNormal, modelview.inverse().transpose())
      .uniformMatrix4x4(U::tex.uTexMatrix, display_tex_matrix ? texmatrix : xform::identity())
      .uniformSampler(U::tex.uTex, 0)
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

    face.draw(vram_size_str, vec2{ FB_DIMS.x - 300.0f, 70.0f }, vec3{ 0.8f, 0.0f, 0.0f });

    small_face.draw(util::fmt("anim_factor: %.2f eye: (%.2f, %.2f, %.2f)",
                              anim_timer.elapsedf(), eye.x, eye.y, eye.z),
                    { 30.0f, 100.0f }, { 1.0f, 1.0f, 1.0f });

    small_face.draw(util::fmt("time: %.8lfs", fps_timer.elapsedSecondsf()),
                    { 30.0f, 100.0f+small_face.height() }, { 1.0f, 1.0f, 1.0f });

    //texmatrix = texmatrix.inverse();
    std::string texmatrix_str =
      util::fmt(
              "%f %f %f %f\n"
              "%f %f %f %f\n"
              "%f %f %f %f\n"
              "%f %f %f %f",
              texmatrix[0], texmatrix[1], texmatrix[2], texmatrix[3],
              texmatrix[4], texmatrix[5], texmatrix[6], texmatrix[7],
              texmatrix[8], texmatrix[9], texmatrix[10], texmatrix[11],
              texmatrix[12], texmatrix[13], texmatrix[14], texmatrix[15]);

    if(display_tex_matrix) small_face.draw(texmatrix_str, vec2{ 30.0f, 150.0f }, vec3{ 1, 1, 1 });

    iface.paint();
    cursor.paint();

    fb.blitToWindow(ivec4{ 0, 0, FB_DIMS.x, FB_DIMS.y }, ivec4{ 0, 0, 1280, 720 },
                    gx::Framebuffer::ColorBit, gx::Sampler::Nearest);

    window.swapBuffers();

    frames++;
  }

  game::finalize();
  ui::finalize();
  ft::finalize();
  gx::finalize();

  win32::finalize();

  return 0;
}