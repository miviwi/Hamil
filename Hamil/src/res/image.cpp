#include <res/image.h>

#include <stb_image/stb_image.h>

namespace res {

Resource::Ptr Image::from_memory(void *buf, size_t sz, unsigned channels, unsigned flags,
  Id id, const std::string& name, const std::string& path)
{
  auto image = new Image(id, Image::tag(), name, Memory, path);

  image->load(buf, sz, channels, flags);

  return Resource::Ptr(image);
}

Resource::Ptr Image::from_file(void *buf, size_t sz, unsigned channels, unsigned flags,
  Id id, const std::string& name, const std::string& path)
{
  auto image = new Image(id, Image::tag(), name, Memory, path);

  image->load(buf, sz, channels, flags);

  return Resource::Ptr(image);
}

void Image::load(void *buf, size_t sz, unsigned channels, unsigned flags)
{
  if(!buf) return;    // TODO: async loading

  stbi_set_flip_vertically_on_load(flags & FlipVertical ? true : false);
  stbi_set_unpremultiply_on_load(flags & Unpremultiply ? true : false);

  m_data = stbi_load_from_memory((byte *)buf, (int)sz, &m_dims.x, &m_dims.y, &m_channels, channels);

  m_loaded = true;
}

Image::~Image()
{
  stbi_image_free(m_data);
}

}