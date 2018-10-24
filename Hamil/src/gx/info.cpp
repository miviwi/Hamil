#include <gx/info.h>

namespace gx {

size_t GxInfo::maxTextureSize() const
{
  return m_max_texture_sz;
}

size_t GxInfo::maxTextureArrayLayers() const
{
  return m_max_array_tex_layers;
}

size_t GxInfo::maxTextureBufferSize() const
{
  return m_max_tex_buffer_sz;
}

size_t GxInfo::maxUniformBlockSize() const
{
  return m_max_uniform_block_sz;
}

size_t GxInfo::minUniformBindAlignment() const
{
  return m_min_uniform_buf_alignment;
}

size_t GxInfo::maxUniformBufferBindings() const
{
  return m_max_uniform_bindings;
}

size_t GxInfo::maxTextureUnits() const
{
  return m_max_tex_image_units;
}

static void p_glGet(GLenum pname, size_t *data)
{
  return glGetInteger64v(pname, (GLint64 *)data);
}

GxInfo *GxInfo::create()
{
  auto self = new GxInfo();

  p_glGet(GL_MAX_TEXTURE_SIZE, &self->m_max_texture_sz);
  p_glGet(GL_MAX_ARRAY_TEXTURE_LAYERS, &self->m_max_array_tex_layers);
  p_glGet(GL_MAX_TEXTURE_BUFFER_SIZE, &self->m_max_tex_buffer_sz);
  p_glGet(GL_MAX_UNIFORM_BLOCK_SIZE, &self->m_max_uniform_block_sz);
  p_glGet(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &self->m_min_uniform_buf_alignment);
  p_glGet(GL_MAX_UNIFORM_BUFFER_BINDINGS, &self->m_max_uniform_bindings);
  p_glGet(GL_MAX_TEXTURE_IMAGE_UNITS, &self->m_max_tex_image_units);

  return self;
}

}