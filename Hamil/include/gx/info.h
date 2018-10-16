#pragma once

#include <gx/gx.h>

namespace gx {

// Gives info on GPU resource limits etc.
//   - use gx::info() (gx/gx.h) to get a global instance
class GxInfo {
public:
  // Estimated max texture size (in one dimension) in texels
  //   - glGet(GL_MAX_TEXTURE_SIZE)
  size_t maxTextureSize() const;

  // Max TextureBuffer size in bytes
  //   - glGet(GL_MAX_TEXTURE_BUFFER_SIZE)
  size_t maxTextureBufferSize() const;

  // Maximum size of one GLSL 'uniform' block in bytes
  //   - glGet(GL_MAX_UNIFORM_BLOCK_SIZE)
  size_t maxUniformBlockSize() const;

protected:
  GxInfo() = default;

  static GxInfo *create();

private:
  friend void init();

  // GL_MAX_TEXTURE_SIZE
  size_t m_max_texture_sz;

  // GL_MAX_TEXTURE_BUFFER_SIZE
  size_t m_max_tex_buffer_sz;

  // GL_MAX_UNIFORM_BLOCK_SIZE
  size_t m_max_uniform_block_sz;
};

}