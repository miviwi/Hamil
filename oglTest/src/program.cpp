#include "program.h"
#include "buffer.h"
#include "vertex.h"

#include <Windows.h>

#include <cstring>
#include <vector>

namespace gx {

GLuint p_last_program = ~0u;

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

unsigned Program::getUniformBlockIndex(const char *name)
{
  return glGetUniformBlockIndex(m, name);
}

void Program::uniformBlockBinding(unsigned block, unsigned index)
{
  glUniformBlockBinding(m, block, index);
}

void Program::uniformBlockBinding(const char *name, unsigned index)
{
  uniformBlockBinding(getUniformBlockIndex(name), index);
}

int Program::getOutputLocation(const char *name)
{
  return glGetFragDataLocation(m, name);
}

Program& Program::use()
{
  if(p_last_program == m) return *this;

  p_last_program = m;
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

Program& Program::uniformMatrix4x4(int location, const mat4& mtx)
{
  glUniformMatrix4fv(location, 1, GL_TRUE, mtx);

  return *this;
}

Program& Program::uniformMatrix3x3(int location, const mat3& mtx)
{
  glUniformMatrix3fv(location, 1, GL_TRUE, mtx);

  return *this;
}

Program& Program::uniformMatrix3x3(int location, const mat4& mtx)
{
  return uniformMatrix3x3(location, mtx.toMat3());
}

Program& Program::uniformBool(int location, bool v)
{
  glUniform1i(location, v);

  return *this;
}

void Program::draw(Primitive p, const VertexArray& vtx, unsigned offset, unsigned num)
{
  vtx.use();

  glDrawArrays(p, offset, num);
}

void Program::draw(Primitive p, const VertexArray& vtx, unsigned num)
{
  draw(p, vtx, 0, num);
}

void Program::draw(Primitive p, const VertexArray& vtx, const IndexBuffer& idx, unsigned offset, unsigned num)
{
  vtx.use();
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
  vtx.use();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idx.m);

  glDrawElementsBaseVertex(p, num, idx.m_type, (void *)(offset * idx.elemSize()), base);
}

void Program::label(const char *lbl)
{
#if !defined(NDEBUG)
  glObjectLabel(GL_PROGRAM, m, strlen(lbl), lbl);
#endif
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

static const char *shader_source[256] = {
  "#version 330 core\n\n"         // comma omitted
  "layout(std140) uniform;\n\n",
  nullptr,
};

Shader::Shader(Type type, const char *source) :
  Shader(type, { source })
{
}

Shader::Shader(Type type, std::initializer_list<const char *> sources)
{
  m = glCreateShader(type);

  memcpy(shader_source+1, sources.begin(), sources.size()*sizeof(const char *));

  glShaderSource(m, sources.size()+1, shader_source, nullptr);
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
