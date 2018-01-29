#pragma once

#include "vmath.h"

#include <GL/glew.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <initializer_list>

namespace gx {

class VertexArray;
class IndexBuffer;

class Shader {
public:
  enum Type {
    Invalid,
    Vertex = GL_VERTEX_SHADER,
    Geometry = GL_GEOMETRY_SHADER,
    Fragment = GL_FRAGMENT_SHADER,
  };

  Shader(Type type, const char *source);
  Shader(Type type, std::initializer_list<const char *> sources);
  Shader(const Shader&) = delete;
  ~Shader();

private:
  friend class Program;

  GLuint m;
};

enum Primitive {
  Points = GL_POINTS,

  Lines = GL_LINES,
  LineLoop = GL_LINE_LOOP,
  LineStrip = GL_LINE_STRIP,

  Triangles = GL_TRIANGLES,
  TriangleFan = GL_TRIANGLE_FAN,
  TriangleStrip = GL_TRIANGLE_STRIP,
};

class Program {
public:
  using OffsetMap = std::unordered_map<std::string, size_t>;

  Program(const Shader& vertex, const Shader& fragment);
  Program(const Shader& vertex, const Shader& geometry, const Shader& fragment);
  Program(const Program&) = delete;
  ~Program();

  // Uses sources autogenerated from *.uniform files
  // in the format
  //            <uniform_name>, <uniform_name>, <uniform_name>....
  // c++ style line-comments are also allowed
  template <typename T>
  void getUniformsLocations(T& klass)
  {
    getUniforms(T::offsets.data(), T::offsets.size(), klass.locations);
  }
  GLint getUniformLocation(const char *name);

  unsigned getUniformBlockIndex(const char *name);
  void uniformBlockBinding(unsigned block, unsigned index);
  void uniformBlockBinding(const char *name, unsigned index);
  const OffsetMap& uniformBlockOffsets(unsigned block);

  int getOutputLocation(const char *name);

  Program& use();

  Program& uniformInt(int location, int i);
  Program& uniformVector3(int location, size_t size, const vec3 *v);
  Program& uniformVector3(int location, vec3 v);
  Program& uniformVector4(int location, size_t size, const vec4 *v);
  Program& uniformVector4(int location, vec4 v);
  Program& uniformMatrix4x4(int location, const mat4& mtx);
  Program& uniformMatrix3x3(int location, const mat3& mtx);
  Program& uniformMatrix3x3(int location, const mat4& mtx);
  Program& uniformBool(int location, bool v);

  void draw(Primitive p, const VertexArray& vtx, unsigned offset, unsigned num);
  void draw(Primitive p, const VertexArray& vtx, unsigned num);
  void draw(Primitive p, const VertexArray& vtx, const IndexBuffer& idx, unsigned offset, unsigned num);
  void draw(Primitive p, const VertexArray& vtx, const IndexBuffer& idx, unsigned num);

  void drawBaseVertex(Primitive p, const VertexArray& vtx, const IndexBuffer& idx,
            unsigned base, unsigned offset, unsigned num);

  void label(const char *lbl);

private:
  void link();
  void getUniforms(const std::pair<std::string, unsigned> *offsets, size_t sz, int locations[]);
  void getUniformBlockOffsets();

  GLuint m;
  static const OffsetMap m_null_offsets;
  std::vector<OffsetMap> m_ubo_offsets;
};

}
