#pragma once

#include <gx/gx.h>

namespace gx {

// Gives info on GPU resource limits etc.
//   - use gx::info() (gx/gx.h) to get a global instance
class GxInfo {
public:
  // Estimated max texture size (in one dimension) in texels >= 1024 
  //   - glGet(GL_MAX_TEXTURE_SIZE)
  size_t maxTextureSize() const;

  // Maximum Texture2DArray layers >= 256
  //   - glGet(GL_MAX_ARRAY_TEXTURE_LAYERS)
  size_t maxTextureArrayLayers() const;

  // Maximum TextureBuffer size in bytes >= 64KB
  //   - glGet(GL_MAX_TEXTURE_BUFFER_SIZE)
  size_t maxTextureBufferSize() const;

  // Maximum size of one GLSL 'uniform' block in bytes >= 16KB
  //   - glGet(GL_MAX_UNIFORM_BLOCK_SIZE)
  size_t maxUniformBlockSize() const;

  // The maximum i+1 which can be passed to gx::tex_unit(i, ...) >= 16
  //   - glGet(GL_MAX_TEXTURE_IMAGE_UNITS)
  size_t maxTextureUnits() const;

protected:
  GxInfo() = default;

  static GxInfo *create();

private:
  friend void init();

  // GL_MAX_TEXTURE_SIZE
  size_t m_max_texture_sz;

  // GL_MAX_ARRAY_TEXTURE_LAYERS
  size_t m_max_array_tex_layers;

  // GL_MAX_TEXTURE_BUFFER_SIZE
  size_t m_max_tex_buffer_sz;

  // GL_MAX_UNIFORM_BLOCK_SIZE
  size_t m_max_uniform_block_sz;

  // GL_MAX_TEXTURE_IMAGE_UNITS
  size_t m_max_tex_image_units;
};

}