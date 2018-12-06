#include <gx/program.h>
#include <gx/buffer.h>
#include <gx/vertex.h>

#include <win32/panic.h>
#include <util/format.h>

#include <cassert>
#include <cstring>
#include <vector>
#include <utility>

namespace gx {

thread_local GLuint p_last_program = ~0u;

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

Program::Program(Program&& other) :
  m(other.m), m_ubo_descs(std::move(other.m_ubo_descs))
{
  other.m = 0;
}

Program::~Program()
{
  if(deref()) return;

  glDeleteProgram(m);
}

Program& Program::operator=(Program&& other)
{
  this->~Program();

  m = other.m;
  m_ubo_descs = std::move(other.m_ubo_descs);

  other.m = 0;

  return *this;
}

GLint Program::getUniformLocation(const char *name)
{
  return glGetUniformLocation(m, name);
}

unsigned Program::getUniformBlockIndex(const char *name)
{
  return glGetUniformBlockIndex(m, name);
}

Program& Program::uniformBlockBinding(unsigned block, unsigned index)
{
  glUniformBlockBinding(m, block, index);

  return *this;
}

Program& Program::uniformBlockBinding(const char *name, unsigned index)
{
  return uniformBlockBinding(getUniformBlockIndex(name), index);
}

const Program::DescriptorMap Program::m_null_descs;
const Program::DescriptorMap& Program::uniformBlockDescriptors(unsigned block)
{
  // Lazy-initialize the DescriptorMapVector
  if(!m_ubo_descs) getUniformBlockDescriptors();

  // If the block index is out of range return an empty map
  return block < m_ubo_descs->size() ? m_ubo_descs->at(block) : m_null_descs;
}

const Program::DescriptorMap& Program::uniformBlockDescriptors(const char *name)
{
  return uniformBlockDescriptors(getUniformBlockIndex(name));
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

Program& Program::uniformSampler(int location, int i)
{
  return uniformInt(location, i);
}

Program& Program::uniformFloat(int location, float f)
{
  glUniform1f(location, f);

  return *this;
}

Program& Program::uniformVector3(int location, size_t size, const vec3 *v)
{
  glUniform3fv(location, (GLsizei)size, (const float *)v);

  return *this;
}

Program& Program::uniformVector3(int location, vec3 v)
{
  glUniform3f(location, v.x, v.y, v.z);

  return *this;
}

Program& Program::uniformVector4(int location, size_t size, const vec4 *v)
{
  glUniform4fv(location, (GLsizei)size, (const float *)v);

  return *this;
}

Program& Program::uniformVector4(int location, vec4 v)
{
  glUniform4f(location, v.x, v.y, v.z, v.w);

  return *this;
}

Program& Program::uniformMatrix4x4(int location, const mat4& mtx, bool transpose)
{
  glUniformMatrix4fv(location, 1, transpose, mtx);

  return *this;
}

Program& Program::uniformMatrix3x3(int location, const mat3& mtx, bool transpose)
{
  glUniformMatrix3fv(location, 1, transpose, mtx);

  return *this;
}

Program& Program::uniformMatrix3x3(int location, const mat4& mtx, bool transpose)
{
  return uniformMatrix3x3(location, mtx.xyz(), transpose);
}

Program& Program::uniformBool(int location, bool v)
{
  glUniform1i(location, v);

  return *this;
}

void Program::draw(Primitive p, const VertexArray& vtx, size_t offset, size_t num)
{
  vtx.use();

  glDrawArrays(p, (int)offset, (GLsizei)num);
}

void Program::draw(Primitive p, const VertexArray& vtx, size_t num)
{
  draw(p, vtx, 0, num);
}

void Program::draw(Primitive p, const IndexedVertexArray& vtx, size_t offset, size_t num)
{
  vtx.use();

  glDrawElements(p, (GLsizei)num, vtx.indexType(), (void *)(offset * vtx.indexSize()));
}

void Program::draw(Primitive p, const IndexedVertexArray& vtx, size_t num)
{
  draw(p, vtx, 0, num);
}

void Program::drawBaseVertex(Primitive p,
 const IndexedVertexArray& vtx, size_t base, size_t offset, size_t num)
{
  vtx.use();

  glDrawElementsBaseVertex(p, (GLsizei)num, vtx.indexType(), (void *)(offset * vtx.indexSize()), (int)base);
}

void Program::label(const char *lbl)
{
#if !defined(NDEBUG)
  use();

  glObjectLabel(GL_PROGRAM, m, -1, lbl);
#endif
}

void Program::unbind()
{
  p_last_program = 0;
  glUseProgram(0);
}

void Program::link()
{
  glLinkProgram(m);

  GLint link_success = 0;
  glGetProgramiv(m, GL_LINK_STATUS, &link_success);

  if(link_success == GL_TRUE) return;

  // An error occurred during linking
  GLint log_size;
  glGetProgramiv(m, GL_INFO_LOG_LENGTH, &log_size);

  std::vector<char> log(log_size);
  glGetProgramInfoLog(m, log_size, nullptr, log.data());

  win32::panic(
    util::fmt("OpenGL program linking error:\n%s", log).data(),
    win32::ShaderLinkingError);
}

void Program::getUniforms(const std::pair<std::string, unsigned> *offsets, size_t sz, int locations[])
{
  for(unsigned i = 0; i < sz; i++) {
    const auto& o = offsets[i];
    GLint loc = glGetUniformLocation(m, o.first.data());

    locations[o.second] = loc;
  }
}

thread_local static char p_uniform_name[256];

void Program::getUniformBlockDescriptors()
{
  enum { InitalNumIndices = 64 };

  std::vector<unsigned> indices(InitalNumIndices);
  std::vector<int> offsets(InitalNumIndices);
  std::vector<int> sizes(InitalNumIndices);
  std::vector<int> strides(InitalNumIndices);

  int num_blocks = 0;
  glGetProgramiv(m, GL_ACTIVE_UNIFORM_BLOCKS, &num_blocks);

  m_ubo_descs.emplace(num_blocks);

  for(int block = 0; block < num_blocks; block++) {
    DescriptorMap& offset_map = m_ubo_descs->at(block);

    int num_uniforms = 0;
    glGetActiveUniformBlockiv(m, block, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &num_uniforms);

    // Check if we have enough space in the vectors
    if(num_uniforms > indices.size()) {
      indices.resize(num_uniforms);
      offsets.resize(num_uniforms);
      sizes.resize(num_uniforms);
      strides.resize(num_uniforms);
    }

    // First query the indices of the Uniforms
    glGetActiveUniformBlockiv(m, block, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, (int *)indices.data());

    // ...then their offsets
    glGetActiveUniformsiv(m, num_uniforms, indices.data(), GL_UNIFORM_OFFSET, offsets.data());
    // ...and the sizes
    glGetActiveUniformsiv(m, num_uniforms, indices.data(), GL_UNIFORM_SIZE, sizes.data());
    // ...and lastly the arrays strides
    glGetActiveUniformsiv(m, num_uniforms, indices.data(), GL_UNIFORM_ARRAY_STRIDE, strides.data());

    for(int i = 0; i < num_uniforms; i++) {
      auto uniform = (unsigned)indices[i];

      auto offset = offsets[i];
      auto size   = sizes[i];
      auto stride = strides[i];

      // Query the name string
      glGetActiveUniformName(m, uniform, sizeof(p_uniform_name), nullptr, p_uniform_name);

      offset_map.insert({
        p_uniform_name,

        UniformDescriptor{ offset, size, stride }
      });
    }
  }
}

thread_local static const char *p_shader_source[256] = {
  "#version 330 core\n\n"         // Concatenate the strings
  "layout(row_major) uniform;\n"
  "layout(std140) uniform;\n\n",
  nullptr,
};

Shader::Shader(Type type, SourcesList sources) :
  Shader(type, sources.data(), sources.size())
{
}

Shader::Shader(Type type, const char *const sources[], size_t count)
{
  m = glCreateShader(type);

  memcpy(p_shader_source+1, sources, count*sizeof(const char *));

  glShaderSource(m, (GLsizei)count+1 /* add the prelude */, p_shader_source, nullptr);
  glCompileShader(m);

  GLint success = 0;
  glGetShaderiv(m, GL_COMPILE_STATUS, &success);

  if(success == GL_TRUE) return;

  // An error occurred during compilation
  GLint log_size;
  glGetShaderiv(m, GL_INFO_LOG_LENGTH, &log_size);

  std::vector<char> log(log_size);
  glGetShaderInfoLog(m, log_size, nullptr, log.data());

  // Concatenate all the sources
  std::string source;
  const char **source_ptr = p_shader_source;
  while(*source_ptr) {
    source += *source_ptr;
    source_ptr++;
  }

  win32::panic(
    util::fmt("OpenGL shader compilation error:\n%s\nSource: %s\n", log, source).data(),
    win32::ShaderCompileError);
}

Shader::~Shader()
{
  glDeleteShader(m);
}

}
