#include "program.h"
#include "buffer.h"
#include "vertex.h"

#include <Windows.h>

#include <vector>

namespace gx {

Program::Program(const Shader& vertex, const Shader& fragment)
{
  m = glCreateProgram();

  glAttachShader(m, vertex.m);
  glAttachShader(m, fragment.m);

  link();
 
  glDetachShader(m, vertex.m);
  glDetachShader(m, fragment.m);
}

Program::Program(const Shader& vertex, const Shader& geometry, const Shader& fragment)
{
  m = glCreateProgram();

  glAttachShader(m, vertex.m);
  glAttachShader(m, geometry.m);
  glAttachShader(m, fragment.m);

  link();

  glDetachShader(m, vertex.m);
  glDetachShader(m, geometry.m);
  glDetachShader(m, fragment.m);
}

Program::~Program()
{
  glDeleteProgram(m);
}

GLint Program::getUniformLocation(const char *name)
{
  return glGetUniformLocation(m, name);
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

Program& Program::uniformVector3(int location, vec3 v)
{
  glUniform3f(location, v.x, v.y, v.z);

  return *this;
}

Program& Program::uniformVector4(int location, size_t size, const vec4 *v)
{
  glUniform4fv(location, size, (const float *)v);

  return *this;
}

Program& Program::uniformVector4(int location, vec4 v)
{
  glUniform4f(location, v.x, v.y, v.z, v.w);

  return *this;
}

Program& Program::uniformMatrix4x4(int location, const float *mtx)
{
  glUniformMatrix4fv(location, 1, GL_TRUE, mtx);

  return *this;
}

void Program::draw(Primitive p, const VertexArray& vtx, unsigned offset, unsigned num)
{
  glBindVertexArray(vtx.m);

  glDrawArrays(p, offset, num);
}

void Program::draw(Primitive p, const VertexArray& vtx, unsigned num)
{
  draw(p, vtx, 0, num);
}

void Program::draw(Primitive p, const VertexArray& vtx, const IndexBuffer& idx, unsigned offset, unsigned num)
{
  glBindVertexArray(vtx.m);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx.m);

  glDrawElements(p, num, idx.m_type, (void *)(offset * idx.elemSize()));
}

void Program::draw(Primitive p, const VertexArray& vtx, const IndexBuffer& idx, unsigned num)
{
  draw(p, vtx, idx, 0, num);
}

void Program::drawBaseVertex(Primitive p, const VertexArray& vtx, const IndexBuffer& idx,
                             unsigned base, unsigned offset, unsigned num)
{
  glBindVertexArray(vtx.m);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx.m);

  glDrawElementsBaseVertex(p, num, idx.m_type, (void *)(offset * idx.elemSize()), base);
}

void Program::link()
{
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
}

void Program::getUniforms(const std::pair<std::string, unsigned> *offsets, size_t sz, int locations[])
{
  for(unsigned i = 0; i < sz; i++) {
    const auto& o = offsets[i];
    GLint loc = glGetUniformLocation(m, o.first.c_str());

    locations[o.second] = loc;
  }
}

Shader::Shader(Type type, const char *source)
{
  m = glCreateShader(type);

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

}
