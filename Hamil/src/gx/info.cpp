#include <gx/info.h>

namespace gx {

size_t GxInfo::maxTextureSize() const
{
  return m_max_texture_sz;
}

size_t GxInfo::maxTextureBufferSize() const
{
  return m_max_tex_buffer_sz;
}

size_t GxInfo::maxUniformBlockSize() const
{
  return m_max_uniform_block_sz;
}

static void p_glGet(GLenum pname, size_t *data)
{
  return glGetInteger64v(pname, (GLint64 *)data);
}

GxInfo *GxInfo::create()
{
  auto self = new GxInfo();

  p_glGet(GL_MAX_TEXTURE_SIZE, &self->m_max_texture_sz);
  p_glGet(GL_MAX_TEXTURE_BUFFER_SIZE, &self->m_max_tex_buffer_sz);
  p_glGet(GL_MAX_UNIFORM_BLOCK_SIZE, &self->m_max_uniform_block_sz);

  return self;
}

}