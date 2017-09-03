#include "program.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vector>

namespace ogl {

Program::Program(const Shader& vertex, const Shader& fragment)
{
  m = glCreateProgram();

  glAttachShader(m, vertex.m);
  glAttachShader(m, fragment.m);

  glLinkProgram(m);

  GLint link_success = 0;
  glGetProgramiv(m, GL_LINK_STATUS, &link_success);

  if(link_success == GL_FALSE) {
    GLint log_size;
    glGetProgramiv(m, GL_LINK_STATUS, &log_size);

    std::vector<char> log(log_size);
    glGetProgramInfoLog(m, log_size, nullptr, log.data());

    MessageBoxA(nullptr, log.data(), "OpenGL program linking error", MB_OK);
    ExitProcess(-2);
  }

  glDetachShader(m, vertex.m);
  glDetachShader(m, fragment.m);
}

Program::~Program()
{
  glDeleteProgram(m);
}

Program& Program::use()
{
  glUseProgram(m);

  return *this;
}

Program& Program::uniformInt(const char *name, int i)
{
  glUniform1i(glGetUniformLocation(m, name), i);

  return *this;
}

Program& Program::uniformMatrix4x4(const char *name, const float *mtx)
{
  glUniformMatrix4fv(glGetUniformLocation(m, name), 1, GL_TRUE, mtx);

  return *this;
}

void Program::drawTraingles(unsigned num)
{
  glDrawArrays(GL_TRIANGLES, 0, num*3);
}

Shader::Shader(Type type, const char *source)
{
  m = glCreateShader(shader_type(type));

  glShaderSource(m, 1, &source, nullptr);
  glCompileShader(m);

  GLint success = 0;
  glGetShaderiv(m, GL_COMPILE_STATUS, &success);

  if(!success) {
    GLint log_size;
    glGetShaderiv(m, GL_INFO_LOG_LENGTH, &log_size);

    std::vector<char> log(log_size);
    glGetShaderInfoLog(m, log_size, nullptr, log.data());

    MessageBoxA(nullptr, log.data(), "OpenGL shader compilation error", MB_OK);
    ExitProcess(-1);
  }
}

Shader::~Shader()
{
  glDeleteShader(m);
}

GLenum Shader::shader_type(Type type)
{
  static const GLenum table[] = {
    ~0u,
    GL_VERTEX_SHADER, GL_FRAGMENT_SHADER,
  };
  return table[type];
}

}
