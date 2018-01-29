#include "window.h"
#include "buffer.h"
#include "vertex.h"
#include "program.h"
#include "uniforms.h"
#include "pipeline.h"
#include "texture.h"
#include "framebuffer.h"
#include "font.h"

#include "ui/ui.h"
#include "ui/frame.h"
#include "ui/layout.h"
#include "ui/button.h"
#include "ui/slider.h"
#include "ui/label.h"
#include "ui/dropdown.h"

#include <GL/glew.h>

#include <vector>
#include <array>
#include <utility>

#include <fcntl.h>
#include <io.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  using win32::Window;
  Window window(1280, 720);

  constexpr ivec2 FB_DIMS{ 1280, 720 };

  //auto map = (glang::MapObject *)o;
  //auto map_b = map->seqget(vm->objMan().kw(":b"));

  ft::init();
  ui::init();

  ft::Font face(ft::FontFamily("georgia"), 35);
  ft::Font small_face(ft::FontFamily("segoeui"), 12);

  int mouse_x = (1280)/2, mouse_y = (720)/2;
  float view_x = 0, view_y = 0;
  float zoom = 1.0f, rot = 0.0f;

  bool constrained = true;

  int animate = -1;

  auto fmt = gx::VertexFormat()
    .attr(gx::f32, 3)
    .attr(gx::f32, 3);
  auto cursor_fmt = gx::VertexFormat()
    .attr(gx::f32, 2)
    .attr(gx::f32, 2);

  gx::VertexBuffer cursor_buf(gx::Buffer::Static);
  gx::VertexArray cursor(cursor_fmt, cursor_buf);

  vec2 cursor_vtx[6] = {
    { 0.0f, 0.0f }, { 0.0f, 2.0f },
    { 0.0f, 0.8f }, { 0.0f, 0.0f },
    { 0.7f, 0.5f }, { 2.0f, 0.0f },
  };

  cursor_buf.init(cursor_vtx, 6);

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
} output;

void main() {
  output.position = vec3(uModelView * vec4(iPosition, 1.0));
  output.normal = uNormal * iNormal;

  gl_Position = uProjection * vec4(output.position, 1.0);
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
} input;

layout(location = 0) out vec3 color;

void main() {
  vec3 normal = normalize(input.normal);

  vec3 light = vec3(0, 0, 0);
  for(int i = 0; i < num_lights; i++) {
    light += phong(lights[i], input.position, normal);
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
} output;

void main() {
  output.position = vec3(uModelView * vec4(iPosition, 0.0, 1.0));
  output.normal = uNormal * vec3(0.0, 0.0, -1.0);
  output.tex_coord = (uTexMatrix * vec4(iTexCoord, 0.0f, 1.0f)).st;

  gl_Position = uProjection * vec4(output.position, 1.0f);
}
)VTX";

  const char *tex_fs_src = R"FG(
uniform sampler2D uTex;

in VertexData {
  vec3 position;
  vec3 normal;
  vec2 tex_coord;
} input;

layout(location = 0) out vec3 color;

void main() {
  vec3 normal = normalize(input.normal); 
  vec4 diffuse = texture(uTex, input.tex_coord);

  vec3 light = vec3(0, 0, 0);
  for(int i = 0; i < num_lights; i++) {
    light += phong(lights[i], input.position, normal);
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

  fb_tex.initMultisample(4, FB_DIMS.x, FB_DIMS.y);
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
  // xform::perspective(59.0f, 16.0f/9.0f, 0.01f, 100.0f);

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
    { { -1.0f, -1.0f, -1.0f }, normals[0] },
    { { 1.0f, -1.0f, -1.0f }, normals[0] },

    { { 1.0f, -1.0f, -1.0f }, normals[0] },
    { { 1.0f, 1.0f, -1.0f }, normals[0] },
    { { -1.0f, 1.0f, -1.0f }, normals[0] },

    // FRONT
    { { -1.0f, 1.0f, 1.0f }, normals[1] },
    { { -1.0f, -1.0f, 1.0f }, normals[1] },
    { { 1.0f, -1.0f, 1.0f }, normals[1] },

    { { 1.0f, -1.0f, 1.0f }, normals[1] },
    { { 1.0f, 1.0f, 1.0f }, normals[1] },
    { { -1.0f, 1.0f, 1.0f }, normals[1] },
    
    // TOP
    { { -1.0f, 1.0f, -1.0f }, normals[2] },
    { { 1.0f, 1.0f, 1.0f }, normals[2] },
    { { -1.0f, 1.0f, 1.0f }, normals[2] },

    { { 1.0f, 1.0f, 1.0f }, normals[2] },
    { { -1.0f, 1.0f, -1.0f }, normals[2] },
    { { 1.0f, 1.0f, -1.0f }, normals[2] },
    
    // BOTTOM
    { { -1.0f, -1.0f, -1.0f }, normals[3] },
    { { 1.0f, -1.0f, 1.0f }, normals[3] },
    { { -1.0f, -1.0f, 1.0f }, normals[3] },

    { { 1.0f, -1.0f, 1.0f }, normals[3] },
    { { -1.0f, -1.0f, -1.0f }, normals[3] },
    { { 1.0f, -1.0f, -1.0f }, normals[3] },

    // LEFT
    { { -1.0f, 1.0f, -1.0f }, normals[4] },
    { { -1.0f, -1.0f, 1.0f }, normals[4] },
    { { -1.0f, -1.0f, -1.0f }, normals[4] },

    { { -1.0f, -1.0f, 1.0f }, normals[4] },
    { { -1.0f, 1.0f, -1.0f }, normals[4] },
    { { -1.0f, 1.0f, 1.0f }, normals[4] },

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

  gx::VertexBuffer floor_vbuf(gx::Buffer::Static);
  floor_vbuf.init(floor_vtxs.data(), floor_vtxs.size());

  gx::VertexArray floor_arr(cursor_fmt, floor_vbuf);

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
           .frame(ui::create<ui::LabelFrame>(iface).caption("Toggle wireframe:"))
           .frame<ui::CheckBoxFrame>(iface, "e")
           .gravity(ui::Frame::Left))
    ;

  auto btn_b = iface.getFrameByName<ui::PushButtonFrame>("b"),
    btn_c = iface.getFrameByName<ui::PushButtonFrame>("c");

  btn_b->caption("Quit Application").onClick([&](auto target) {
    window.quit();
  });
  btn_c->caption("Show texmatrix!").onClick([&](auto target) {
    display_tex_matrix = !display_tex_matrix;
    target->caption(display_tex_matrix ? "Hide texmatrix!" : "Show texmatrix!");
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

  while(window.processMessages()) {
    mat4 imtx = mat4{
      1.0f/zoom, 0.0f, 0.0f, -zoom_mtx.d[3]/zoom,
      0.0f, 1.0f/zoom, 0.0f, -zoom_mtx.d[7]/zoom,
      0.0f, 0.0f,            1.0f, 0.0f,
      0.0f,                  0.0f, 1.0f,
    }*xform::translate(-view_x, -view_y, 0);

    vec4 m = imtx * vec4{ (float)mouse_x, (float)mouse_y, 0, 1 };

    constexpr auto anim_time = 10000.0f;
    auto time = GetTickCount();

    while(auto input = window.getInput()) {
      if(iface.input({ mouse_x, mouse_y }, input)) {
        auto mouse = (win32::Mouse *)input.get();
        mouse_x += mouse->dx; mouse_y += mouse->dy;

        continue;
      }

      if(input->getTag() == win32::Keyboard::tag()) {
        using win32::Keyboard;
        auto kb = (Keyboard *)input.get();

        if(kb->keyDown('U')) {
          animate = animate < 0 ? time : -1;
        } else if(kb->keyDown('N')) {
          normals[0] = normals[0]+vec3{ 0.05f, 0.05f, 0.05f };
        } else if(kb->keyDown('Q')) {
          window.quit();
        } else if(kb->keyDown('O')) {
          ortho_projection = !ortho_projection;
        }
      } else {
        using win32::Mouse;
        auto mouse = (Mouse *)input.get();

        mouse_x += mouse->dx;
        mouse_y += mouse->dy;

        if(mouse_x < 0) mouse_x = 0;
        if(mouse_y < 0) mouse_y = 0;

        if(mouse->buttons & Mouse::Right) {
          view_x += mouse->dx;
          view_y += mouse->dy;
        } else if(mouse->event == Mouse::Wheel) {
          zoom += (mouse->ev_data/120)*0.05f;

          zoom_mtx =
            xform::translate(m.x, m.y, 0)
            *xform::scale(zoom, zoom, 1)
            *xform::translate(-m.x, -m.y, 0);
        } else if(mouse->buttonDown(Mouse::Middle)) {
          zoom = 1.0f;

          zoom_mtx = xform::identity();
        }
      }
    }

    if(checkbox.value()) {
      pipeline.wireframe();
    } else {
      pipeline.filledPolys();
    }

    gx::tex_unit(0, tex, sampler);

    fb.use();
    pipeline.use();

    gx::clear(gx::Framebuffer::ColorBit|gx::Framebuffer::DepthBit);

    if(animate > 0) {
      rot_mtx = xform::translate(1280/2.0f, 720.0f/2.0f, 0)
        *xform::rotz(lerp(0.0f, 3.1415f, (float)((time-animate)%(int)anim_time)/(anim_time/2.0f)))
        *xform::translate(-1280.0f/2.0f, -720.0f/2.0f, 0)
        ;
    } else {
      rot_mtx = xform::identity();
    }

    mat4 model =
      xform::translate(view_x, view_y, -50.0f)
      *zoom_mtx
      *rot_mtx
      ;

    mat4 cursor_mtx =
      xform::translate(mouse_x, mouse_y, -1.0f)
      *xform::scale(16.0f, 16.0f, 1.0f)
      //*xform::rotz(11*3.1415/6.0f)
      ;

    float anim_factor = ((time-animate)%(int)anim_time)/(anim_time/2.0f);

    auto persp = !ortho_projection ? xform::perspective(70, 16./9., 0.1f, 1000.0f) :
      xform::ortho(9.0f, -16.0f, -9.0f, 16.0f, 0.1f, 1000.0f);

    vec3 eye{ (float)view_x/1280.0f*150.0f, (float)view_y/720.0f*150.0f, 60.0f/zoom };
    auto view = xform::look_at(eye, vec3{ 0.0f, 0.0f, 0.0f }, vec3{ 0, 1, 0 });

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
      *xform::roty(lerp(0.0, PI, anim_factor))
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
      *xform::rotx(PI/2.0f)
      ;

    mat4 texmatrix = xform::identity()
      *xform::translate(5.0f, 5.0f, 0.0f)
      *xform::rotz(lerp(0.0f, PIf, anim_factor))
      *xform::scale(1.0f/(sin((float)time/1000.0f * PI/2.0f) + 2.0f))
      *xform::translate(-5.0f, -5.0f, 0.0f)
      ;

    mat4 modelview = view*model;

    gx::tex_unit(0, tex, floor_sampler);
    tex_program.use()
      .uniformMatrix4x4(U::tex.uProjection, persp)
      .uniformMatrix4x4(U::tex.uModelView, modelview)
      .uniformMatrix3x3(U::tex.uNormal, modelview.inverse().transpose())
      .uniformMatrix4x4(U::tex.uTexMatrix, /*texmatrix*/ xform::identity())
      .uniformInt(U::tex.uTex, 0)
      .draw(gx::Triangles, floor_arr, floor_vtxs.size());

    gx::Pipeline()
      .additiveBlend()
      .depthTest(gx::Pipeline::LessEqual)
      .use();
    for(int i = 0; i < light_block.num_lights; i++) {
      color = { 0, 0, 0, (float)i };
      model = xform::identity()
        *xform::translate(light_position[i])
        *xform::scale(0.2f)
        ;
      drawcube();
    }

    int fps = (float)frames / ((float)(time-start_time) / 1000.0f);

    char str[256];
    sprintf_s(str, "FPS: %d", fps);

    face.draw(str, vec2{ 30.0f, 70.0f }, vec4{ 0.8f, 0.0f, 0.0f, 1.0f });

    face.draw(vram_size_str, vec2{ FB_DIMS.x - 300.0f, 70.0f }, vec3{ 0.8f, 0.0f, 0.0f });

    sprintf_s(str, "anim_factor: %.2f eye: (%.2f, %.2f, %.2f)", anim_factor, eye.x, eye.y, eye.z);
    small_face.draw(str, vec2{ 30.0f, 100.0f }, vec4{ 0.0f, 0.0f, 0.0f, 1.0f });

    texmatrix = texmatrix.inverse();
    char buf[1024];
    sprintf_s(buf,
              "%f %f %f %f\n"
              "%f %f %f %f\n"
              "%f %f %f %f\n"
              "%f %f %f %f",
              texmatrix[0], texmatrix[1], texmatrix[2], texmatrix[3],
              texmatrix[4], texmatrix[5], texmatrix[6], texmatrix[7],
              texmatrix[8], texmatrix[9], texmatrix[10], texmatrix[11],
              texmatrix[12], texmatrix[13], texmatrix[14], texmatrix[15]);

    if(display_tex_matrix) small_face.draw(buf, vec2{ 30.0f, 150.0f }, vec3{ 0, 0, 0 });

    iface.paint();
    
    sprintf_s(str, "(%d, %d)", mouse_x, mouse_y);
    small_face.draw(str, vec2{ (float)mouse_x, (float)mouse_y }, vec4{ 0, 0, 0, 1 });

    //gx::Pipeline().alphaBlend().use();

    color = { 0, 0, 0, 1 };
    program.use()
      .uniformMatrix4x4(U::program.uProjection, ortho)
      .uniformMatrix4x4(U::program.uModelView, cursor_mtx)
      .uniformVector4(U::program.uCol, color)
      .draw(gx::Triangles, cursor, 3);

    fb.blitToWindow(ivec4{ 0, 0, FB_DIMS.x, FB_DIMS.y }, ivec4{ 0, 0, 1280, 720 },
                    gx::Framebuffer::ColorBit, gx::Sampler::Nearest);

    window.swapBuffers();

    frames++;
  }

  ft::finalize();

  return 0;
}