#pragma once

#include "util/staticstring.h"
#include <res/resource.h>
#include <res/io.h>

#include <util/dds.h>

namespace yaml {
class Document;
}

namespace res {

class Texture : public Resource {
public:
  static const Tag tag() { return "texture"; }

  static Resource::Ptr from_yaml(IOBuffer& image,
    const yaml::Document& doc, Id id,
    const std::string& name = "", const std::string& path = "");

  const util::DDSImage& get() const;

protected:
  using Resource::Resource;

private:
  friend ResourceManager;

  util::DDSImage m_tex;
};

}
