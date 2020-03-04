#pragma once

#include <res/resource.h>

#include <math/geometry.h>

#include <memory>

namespace res {

class Image : public Resource {
public:
  static const Tag tag() { return "image"; }

  enum LoadFlags : unsigned {
    FlipVertical  = 1<<0,
    Unpremultiply = 1<<1,
  };

  virtual ~Image();

  static Resource::Ptr from_memory(void *buf, size_t sz, unsigned channels, unsigned flags, Id id,
    const std::string& name = "", const std::string& path = "");

  static Resource::Ptr from_file(void *buf, size_t sz, unsigned channels, unsigned flags, Id id,
    const std::string& name = "", const std::string& path = "");

  int channels() const { return m_channels; }
  ivec2 dimensions() const { return m_dims; }

  int width() const  { return m_dims.x; }
  int height() const { return m_dims.y; }

  template <typename T = void>
  T *data() const
  {
    return (T *)m_data;
  }

protected:
  using Resource::Resource;

private:
  friend class ResourceManager;

  void load(void *buf, size_t sz, unsigned channels, unsigned flags);

  int m_channels;
  ivec2 m_dims;
  void *m_data;
};

}
