#pragma once

#include "vmath.h"

#include <GL/glew.h>

#include <string>
#include <unordered_map>

namespace gx {

class VertexArray;
class IndexBuffer;

class Shader {
public:
  enum Type {
    Invalid,
    Vertex, Fragment,
  };

  Shader(Type type, const char *source);
  Shader(const Shader&) = delete;
  ~Shader();

private:
  friend class Program;

  static GLenum shader_type(Type type);

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
  Program(const Shader& vertex, const Shader& fragment);
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

  Program& use();

  Program& uniformInt(int location, int i);
  Program& uniformVector3(int location, size_t size, const vec3 *v);
  Program& uniformVector3(int location, vec3 v);
  Program& uniformVector4(int location, size_t size, const vec4 *v);
  Program& uniformVector4(int location, vec4 v);
  Program& uniformMatrix4x4(int location, const float *mtx);

  void draw(Primitive p, const VertexArray& vtx, unsigned offset, unsigned num);
  void draw(Primitive p, const VertexArray& vtx, unsigned num);
  void draw(Primitive p, const VertexArray& vtx, const IndexBuffer& idx, unsigned offset, unsigned num);
  void draw(Primitive p, const VertexArray& vtx, const IndexBuffer& idx, unsigned num);

private:
  void getUniforms(const std::pair<std::string, unsigned> *offsets, size_t sz, int locations[]);

  GLuint m;
};

}
