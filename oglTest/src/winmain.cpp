#include "application.h"
#include "buffer.h"
#include "vertex.h"
#include "program.h"
#include "texture.h"

#define GLEW_STATIC
#include <Gl/glew.h>

#include <vector>
#include <array>
#include <utility>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  using win32::Application;
  Application app(1280, 720);

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

  bool constrained = true;

  auto fmt = ogl::VertexFormat().attr(ogl::VertexFormat::f32, 2);
  auto cursor_fmt = ogl::VertexFormat()
    .attr(ogl::VertexFormat::f32, 2)
    .attr(ogl::VertexFormat::f32, 2);

  ogl::VertexBuffer buf(ogl::VertexBuffer::Stream);
  ogl::VertexArray vtx_array(fmt, buf);

  ogl::VertexBuffer cursor_buf(ogl::VertexBuffer::Static);
  ogl::VertexArray cursor(cursor_fmt, cursor_buf);

  vec2 cursor_vtx[6] = {
    { 0.0f, 0.0f }, { 0.0f, 0.0f },
    { 0.0f, 0.8f }, { 0.0f, 1.5f },
    { 0.7f, 0.5f }, { 1.5f, 1.5f },
  };

  cursor_buf.init(cursor_vtx, sizeof(vec2), 6);

  unsigned char tex_image[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  ogl::Texture tex(ogl::Texture::rgb);
  ogl::Sampler sampler;

  sampler.param(ogl::Sampler::WrapS, ogl::Sampler::Repeat);
  sampler.param(ogl::Sampler::WrapT, ogl::Sampler::Repeat);

  sampler.param(ogl::Sampler::MinFilter, ogl::Sampler::Nearest);
  sampler.param(ogl::Sampler::MagFilter, ogl::Sampler::Nearest);

  tex.init(tex_image, 0, 4, 4, ogl::Texture::rgb, ogl::Texture::u8);

  ogl::tex_unit(0, tex, sampler);

  unsigned cp = 0;

  const char *vs_src = R"VTX(
#version 330

uniform mat4 uModelView;
uniform mat4 uProjection;

layout(location = 0) in vec2 iPosition;

out vec3 Color;

void main() {
  switch(gl_VertexID % 3) {
  case 0: Color = vec3(1.0f, 0.5f, 0.0f); break;
  case 1: Color = vec3(0.0f, 1.0f, 1.0f); break;
  case 2: Color = vec3(0.6f, 0.0f, 1.0f); break;
  }

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

  ogl::Shader vtx_shader(ogl::Shader::Vertex, vs_src);
  ogl::Shader frag_shader(ogl::Shader::Fragment, fs_src);

  ogl::Shader cursor_vtx_shader(ogl::Shader::Vertex, cursor_vs_src);
  ogl::Shader cursor_frag_shader(ogl::Shader::Fragment, cursor_fs_src);

  ogl::Program program(vtx_shader, frag_shader);
  ogl::Program cursor_program(cursor_vtx_shader, cursor_frag_shader);

  glViewport(0, 0, 1280, 720);

  float r = 1280.0f;
  float b = 720.0f;

  mat4 projection = xform::ortho(0, 0, b, r, 0, 1);

  mat4 zoom_mtx = xform::identity();
  mat4 rot_mtx = xform::identity();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  app.captureMouse();
  
  while(app.processMessages()) {
    while(auto input = app.getInput()) {
      if(input->getTag() == win32::Keyboard::tag()) {
        using win32::Keyboard;
        auto kb = (Keyboard *)input.get();

        if(kb->keyDown('A')) {
          char buf[1024];
          sprintf_s(buf, "%f %f %f %f\n"
                    "%f %f %f %f\n"
                    "%f %f %f %f\n"
                    "%f %f %f %f",

                    zoom_mtx[0], zoom_mtx[1], zoom_mtx[2], zoom_mtx[3],
                    zoom_mtx[4], zoom_mtx[5], zoom_mtx[6], zoom_mtx[7],
                    zoom_mtx[8], zoom_mtx[9], zoom_mtx[10], zoom_mtx[11],
                    zoom_mtx[12], zoom_mtx[13], zoom_mtx[14], zoom_mtx[15]);

          MessageBoxA(nullptr, buf, "zoom_mtx", MB_OK);
        }
      } else {
        using win32::Mouse;
        auto mouse = (Mouse *)input.get();

        mouse_x += mouse->dx;
        mouse_y += mouse->dy;

        if(mouse->buttonDown(Mouse::Left) && zoom == 1.0f) {
          auto& pp = p[cp];

          pp.x = mouse_x - view_x;
          pp.y = mouse_y - view_y;

          cp = (cp + 1)%3;

          if(!cp) {
            trigs.push_back(p);

            buf.init(trigs.data(), sizeof(Triangle), trigs.size());
          }
        } else if(mouse->buttons & Mouse::Right) {
          view_x += mouse->dx;
          view_y += mouse->dy;
        } else if(mouse->event == Mouse::Wheel) {
          zoom += (mouse->ev_data/120)*0.05f;

          ivec2 d = { mouse_x-view_x, mouse_y-view_y };

          zoom_mtx =
            xform::translate(d.x, d.y, 0)
            *xform::scale(zoom, zoom, 1)
            *xform::translate(-d.x, -d.y, 0);
        } else if(mouse->buttonDown(Mouse::Middle)) {
          zoom = 1.0f;

          zoom_mtx = xform::identity();
        }
      }
    }

    glClearColor(0.25f, 0.25f, 0.25f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glLineWidth(2.0f);
    glPointSize(4.0f);

    mat4 modelview =
      xform::translate(view_x, view_y, 0)
      *zoom_mtx
      *rot_mtx
      ;

    mat4 cursor_mtx =
      xform::translate(mouse_x, mouse_y, 0)
      *xform::scale(16.0f, 16.0f, 1.0f)
      //*xform::rotz(11*3.1415/6.0f)
      ;

    vtx_array.use();
    program.use()
      .uniformMatrix4x4("uProjection", projection)
      .uniformMatrix4x4("uModelView", modelview)
      .drawTraingles(trigs.size());

    cursor.use();
    cursor_program.use()
      .uniformMatrix4x4("uProjection", projection)
      .uniformMatrix4x4("uModelView", cursor_mtx)
      .uniformInt("uTex", 0)
      .drawTraingles(1);

    app.swapBuffers();
  }

  return 0;
}