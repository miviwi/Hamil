#pragma once

#include <GL/glew.h>

namespace ogl {

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

class Program {
public:
  Program(const Shader& vertex, const Shader& fragment);
  Program(const Program&) = delete;
  ~Program();

  Program& use();

  Program& uniformInt(const char *name, int i);
  Program& uniformMatrix4x4(const char *name, const float *mtx);

  void drawTraingles(unsigned num);

private:
  GLuint m;
};

}
