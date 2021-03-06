#include <gx/gx.h>
#include <gx/info.h>

#include <memory>

namespace gx {

thread_local GLuint p_last_array = ~0u;
GLuint p_dummy_vao;

std::unique_ptr<GxInfo> p_info;

bool is_color_format(Format fmt)
{
  switch(fmt) {
  case depth:
  case depth_stencil:
  case depth16: case depth24: case depth32: case depth32f:
  case depth24_stencil8:
    return false;
  }

  return true;
}

bool is_compressed_format(Format fmt)
{
  switch(fmt) {
  case dxt1: case dxt1_rgba: case dxt1_srgb: case dxt1_srgb_alpha:
  case dxt3: case dxt3_srgb:
  case dxt5: case dxt5_srgb:

  case rgtc1: case rgtc1s: case rgtc2: case rgtc2s:

  case bptc_uf: case bptc_sf:
  case bptc: case bptc_srgb_alpha:
    return true;
  }

  return false;
}

void init()
{
  // 'p_dummy_vao' is created beacuse the GL spec forbids binding
  // to the ELEMENT_ARRAY_BUFFER target without a valid VertexArray bound.
  //
  // This limitation would make constructing gx::IndexBuffer objects error-prone
  // without the workaround.
  glGenVertexArrays(1, &p_dummy_vao);
  p_bind_VertexArray(p_dummy_vao);

#if !defined(NDEBUG)
  glObjectLabel(GL_VERTEX_ARRAY, p_dummy_vao, 1, "$");
#endif

  p_info.reset(GxInfo::create());
}

void finalize()
{
  glDeleteVertexArrays(1, &p_dummy_vao);

  p_info.reset();
}

GxInfo& info()
{
  return *p_info;
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