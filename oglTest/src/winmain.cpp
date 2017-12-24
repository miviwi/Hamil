#include "window.h"
#include "buffer.h"
#include "vertex.h"
#include "program.h"
#include "pipeline.h"
#include "texture.h"
#include "framebuffer.h"
#include "font.h"

#include <Gl/glew.h>

#include <vector>
#include <array>
#include <utility>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  using win32::Window;
  Window window(1280, 720);

  constexpr ivec2 FB_DIMS{ 1280, 720 };

  ft::init();

  ft::Font face(ft::FontFamily("times"), 35);
  ft::Font small_face(ft::FontFamily("times"), 18);

  struct Triangle {
    vec2 a, b, c;

    Triangle(vec2 a_, vec2 b_, vec2 c_) : a(a_), b(b_), c(c_) { }
    Triangle(vec2 p[3]) : a(p[0]), b(p[1]), c(p[2]) { }
  };

  vec2 p[3] = {
    { 0.0f, 0.0f },
    { 1280.0f, 720.0f },
    { 0.0f, 720.0f },
  };

  std::vector<Triangle> trigs;

  int mouse_x = (1280)/2, mouse_y = (720)/2;
  float view_x = 0, view_y = 0;
  float zoom = 1.0f, rot = 0.0f;

  vec3 colors[3] = {
    {1.0f, 0.5f, 0.0f},
    {0.0f, 1.0f, 1.0f},
    {0.6f, 0.0f, 1.0f},
  };

  bool constrained = true;

  int animate = -1;

  auto fmt = gx::VertexFormat()
    .attr(gx::VertexFormat::f32, 2);
  auto cursor_fmt = gx::VertexFormat()
    .attr(gx::VertexFormat::f32, 2)
    .attr(gx::VertexFormat::f32, 2);

  gx::VertexBuffer buf(gx::Buffer::Stream);
  gx::VertexArray vtx_array(fmt, buf);

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

    //0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    //0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    //0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00,
    //0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00,
  };

  gx::Texture2D tex(gx::Texture2D::rgb);
  gx::Sampler sampler;

  sampler.param(gx::Sampler::WrapS, gx::Sampler::Repeat);
  sampler.param(gx::Sampler::WrapT, gx::Sampler::Repeat);

  sampler.param(gx::Sampler::MinFilter, gx::Sampler::Linear);
  sampler.param(gx::Sampler::MagFilter, gx::Sampler::Linear);

  tex.init(tex_image, 0, 4, 4, gx::Texture2D::rgba, gx::Texture2D::u32_8888);

  gx::Texture2D fb_tex(gx::Texture2D::rgba8);
  gx::Framebuffer fb;

  fb_tex.init(FB_DIMS.x, FB_DIMS.y);

  fb.use();
  fb.tex(fb_tex, 0, gx::Framebuffer::Color0);
  fb.renderbuffer(gx::Framebuffer::depth24, gx::Framebuffer::Depth);

  gx::tex_unit(0, tex, sampler);

  unsigned cp = 0;

  const char *vs_src = R"VTX(
#version 330

uniform mat4 uModelView;
uniform mat4 uProjection;

uniform vec3 uCol[3];

layout(location = 0) in vec2 iPosition;

out vec3 Color;

void main() {
/*
  switch(gl_VertexID % 3) {
  case 0: Color = vec3(1.0f, 0.5f, 0.0f); break;
  case 1: Color = vec3(0.0f, 1.0f, 1.0f); break;
  case 2: Color = vec3(0.6f, 0.0f, 1.0f); break;
  }
*/

  Color = uCol[gl_VertexID % 3];

  gl_Position = uProjection * uModelView * vec4(iPosition, 0.0f, 1.0f);
}
)VTX";

  const char *fs_src = R"FG(
#version 330

in vec3 Color;

out vec4 oColor;

void main() {
  oColor = vec4(Color, 1.0f);
}
)FG";

  const char *cursor_vs_src = R"VTX(
#version 330

uniform mat4 uModelView;
uniform mat4 uProjection;

layout(location = 0) in vec2 iPosition;
layout(location = 1) in vec2 iTexCoord;

out vec2 TexCoord;

void main() {
  TexCoord = iTexCoord;
  gl_Position = uProjection * uModelView * vec4(iPosition, 0.0f, 1.0f);
}
)VTX";

  const char *cursor_fs_src = R"FG(
#version 330

uniform sampler2D uTex;

in vec2 TexCoord;

out vec4 oColor;

void main() {
  oColor = texture(uTex, TexCoord);
}
)FG";

  gx::Shader vtx_shader(gx::Shader::Vertex, vs_src);
  gx::Shader frag_shader(gx::Shader::Fragment, fs_src);

  gx::Shader cursor_vtx_shader(gx::Shader::Vertex, cursor_vs_src);
  gx::Shader cursor_frag_shader(gx::Shader::Fragment, cursor_fs_src);

  gx::Program program(vtx_shader, frag_shader);
  gx::Program cursor_program(cursor_vtx_shader, cursor_frag_shader);

  program.getUniformsLocations(U::program);
  cursor_program.getUniformsLocations(U::cursor);

  auto pipeline = gx::Pipeline()
    .viewport(0, 0, FB_DIMS.x, FB_DIMS.y)
    .depthTest(gx::Pipeline::LessEqual)
    .clear(vec4{ 0.2f, 0.2f, 0.2f, 1.0f }, 1.0f);

  float r = 1280.0f;
  float b = 720.0f;

  mat4 projection = xform::ortho(0, 0, b, r, 0.1f, 100.0f);
   // xform::perspective(59.0f, 16.0f/9.0f, 0.01f, 100.0f);

  mat4 zoom_mtx = xform::identity();
  mat4 rot_mtx = xform::identity();

  window.captureMouse();

  int frames = 0;
  auto start_time = GetTickCount();

  auto hello = small_face.string("gggg Hello, sailor! gggg \nTeraz wchodzimy w nowe millenium"),
    cursor_label = small_face.string("Cursor");

  bool display_zoom_mtx = false;

  std::vector<vec2> vtxs = {
    { -1.0f, 1.0f },
    { -1.0f, -1.0f },
    { 1.0f, -1.0f },

    { -1.0f, 1.0f },
    { 1.0f, 1.0f },
    { 1.0f, -1.0f },
  };

  gx::VertexBuffer vbuf(gx::Buffer::Static);
  vbuf.init(vtxs.data(), vtxs.size());

  gx::VertexArray arr(fmt, vbuf);
  
  while(window.processMessages()) {
    mat4 imtx = mat4{
      1.0f/zoom, 0.0f, 0.0f, -zoom_mtx.d[3]/zoom,
      0.0f, 1.0f/zoom, 0.0f, -zoom_mtx.d[7]/zoom,
      0.0f, 0.0f,            1.0f, 0.0f,
      0.0f,                  0.0f, 1.0f,
    }*xform::translate(-view_x, -view_y, 0);

    vec4 m = imtx * vec4{ (float)mouse_x, (float)mouse_y, 0, 1 };

    constexpr auto anim_time = 5000.0f;
    auto time = GetTickCount();

    while(auto input = window.getInput()) {
      if(input->getTag() == win32::Keyboard::tag()) {
        using win32::Keyboard;
        auto kb = (Keyboard *)input.get();

        if(kb->keyDown('A')) {
          display_zoom_mtx = !display_zoom_mtx;
        } else if(kb->keyDown('U')) {
          animate = animate < 0 ? time : -1;
        } else if(kb->keyDown('N')) {
          colors[0] = colors[0]+vec3{ 0.05f, 0.05f, 0.05f };
        } else if(kb->keyDown('W')) {
          if(!pipeline.isEnabled(gx::Pipeline::Wireframe)) pipeline.wireframe();
          else pipeline.filledPolys();
        }
      } else {
        using win32::Mouse;
        auto mouse = (Mouse *)input.get();

        mouse_x += mouse->dx;
        mouse_y += mouse->dy;

        if(mouse->buttonDown(Mouse::Left)) {
          auto& pp = p[cp];

          pp.x = m.x;
          pp.y = m.y;

          cp = (cp + 1)%3;

          if(!cp) {
            trigs.push_back(p);

            buf.init(trigs.data(), trigs.size());
          }
        } else if(mouse->buttons & Mouse::Right) {
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

    mat4 modelview =
      xform::translate(view_x, view_y, -1.0f)
      *zoom_mtx
      *rot_mtx
      ;

    mat4 cursor_mtx =
      xform::translate(mouse_x, mouse_y, -1.0f)
      *xform::scale(16.0f, 16.0f, 1.0f)
      //*xform::rotz(11*3.1415/6.0f)
      ;

    program.use()
      .uniformMatrix4x4(U::program.uProjection, projection)
      .uniformMatrix4x4(U::program.uModelView, modelview)
      .uniformVector3(U::program.uCol, 3, colors)
      .drawTraingles(vtx_array, trigs.size());

    auto persp = xform::perspective(70, 16./9., 0.1f, 100.0f);
    //modelview = xform::roty(PI/4.0f);
    modelview = xform::translate(0.0f, 0.0f, -20.0f)
      *xform::scale(1.0f, 1.0f, 10.0f)
      *xform::roty(lerp(0.0, PI, (float)((time-animate)%(int)anim_time)/(anim_time/2.0f)))
      *xform::rotz(lerp(0.0, PI, (float)((time-animate)%(int)anim_time)/(anim_time/2.0f)))
      ;

    program.use()
      .uniformMatrix4x4(U::program.uProjection, persp)
      .uniformMatrix4x4(U::program.uModelView, modelview)
      .uniformVector3(U::program.uCol, 3, colors)
      .drawTraingles(arr, vtxs.size()/3);

    gx::tex_unit(0, tex, sampler);

    cursor_program.use()
      .uniformMatrix4x4(U::cursor.uProjection, projection)
      .uniformMatrix4x4(U::cursor.uModelView, cursor_mtx)
      .uniformInt(U::cursor.uTex, 0)
      .drawTraingles(cursor, 1);

    int fps = (float)frames / ((float)(time-start_time) / 1000.0f);

    char str[256];
    sprintf_s(str, "FPS: %d", fps);

    face.draw(str, vec2{ 30.0f, 70.0f }, vec4{ 0.8f, 0.0f, 0.0f, 1.0f });

    {
      //gx::ScopedPipeline p(gx::Pipeline().scissor(30, 590, 300, 30));

      small_face.draw(hello, vec2{ 30.0f, 100.0f }, vec4{ 0.8f, 0.5f, 0.0f, 1.0f });
    }

    sprintf_s(str, "(%d, %d)", mouse_x, mouse_y);
    small_face.draw(str, vec2{ (float)mouse_x, (float)mouse_y }, vec4{ 0, 0, 0, 1 });

    char buf[1024];
    sprintf_s(buf,
              "%f %f %f %f\n"
              "%f %f %f %f\n"
              "%f %f %f %f\n"
              "%f %f %f %f",
              zoom_mtx[0], zoom_mtx[1], zoom_mtx[2], zoom_mtx[3],
              zoom_mtx[4], zoom_mtx[5], zoom_mtx[6], zoom_mtx[7],
              zoom_mtx[8], zoom_mtx[9], zoom_mtx[10], zoom_mtx[11],
              zoom_mtx[12], zoom_mtx[13], zoom_mtx[14], zoom_mtx[15]);

    //MessageBoxA(nullptr, buf, "zoom_mtx", MB_OK);
    if(display_zoom_mtx) small_face.draw(buf, vec2{ 30.0f, 150.0f }, vec3{ 0, 0, 0 });

    //face.drawString("ala ma kota a moze i dwa va av Ma Ta Tb Tc", vec2{ 30.0f, 130.0f },
      //              vec4{ 0.0f, 0.0f, 0.0f, 1.0f });

    fb.blitToWindow(ivec4{ 0, 0, FB_DIMS.x, FB_DIMS.y }, ivec4{ 0, 0, 1280, 720 },
                    gx::Framebuffer::ColorBit, gx::Sampler::Nearest);

    window.swapBuffers();

    frames++;
  }

  ft::finalize();

  return 0;
}