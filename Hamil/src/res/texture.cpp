#include <res/texture.h>

#include <yaml/document.h>
#include <yaml/node.h>

namespace res {

Resource::Ptr Texture::from_yaml(IOBuffer& image,
  const yaml::Document& doc, Id id,
  const std::string& name, const std::string& path)
{
  auto self = new Texture(id, Texture::tag(), name, File, path);

  self->m_tex.load(image.get(), image.size(), util::DDSImage::LoadDefault);
  self->m_loaded = true;

  return Resource::Ptr(self);
}

const util::DDSImage& Texture::get() const
{
  return m_tex;
}

}