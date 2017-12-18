#include "program.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vector>

namespace gx {

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

Program& Program::uniformInt(int location, int i)
{
  glUniform1i(location, i);

  return *this;
}

Program& Program::uniformVector3(int location, size_t size, const vec3 *v)
{
  glUniform3fv(location, size, (const float *)v);

  return *this;
}

Program& Program::uniformMatrix4x4(int location, const float *mtx)
{
  glUniformMatrix4fv(location, 1, GL_TRUE, mtx);

  return *this;
}

void Program::drawTraingles(unsigned num)
{
  glDrawArrays(GL_TRIANGLES, 0, num*3);
}

void Program::getUniforms(const std::unordered_map<std::string, unsigned>& offsets, int locations[])
{
  for(auto& o : offsets) {
    GLint loc = glGetUniformLocation(m, o.first.c_str());

    locations[o.second] = loc;
  }
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
