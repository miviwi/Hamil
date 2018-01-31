#include <gx/gx.h>

namespace gx {

GLuint p_last_array = ~0u;
GLuint p_dummy_vao;

void init()
{
  glGenVertexArrays(1, &p_dummy_vao);
  p_bind_VertexArray(p_dummy_vao);

#if !defined(NDEBUG)
  glObjectLabel(GL_VERTEX_ARRAY, p_dummy_vao, 1, "$");
#endif
}

void finalize()
{
  glDeleteVertexArrays(1, &p_dummy_vao);
}

void p_bind_VertexArray(unsigned array)
{
  if(p_last_array != array) glBindVertexArray(array);

  p_last_array = array;
}

unsigned p_unbind_VertexArray()
{
  unsigned last = p_last_array;

  p_bind_VertexArray(p_dummy_vao);

  return last;
}

}