#pragma once

#include <res/resource.h>
#include <res/io.h>

#include <util/staticstring.h>

#include <vector>

namespace yaml {
class Document;
}

namespace res {

class ResourceManager;

class LookupTable : public Resource {
public:
  static constexpr Tag tag() { return "lut"; }

  enum DataType {
    r16f, r32f,
    rgba32f,
  };

  struct Error { };

  static Resource::Ptr from_yaml(IOBuffer lut_data,
    const yaml::Document& doc, Id id,
    const std::string& name = "", const std::string& path = "");
  
  DataType type() const;

  void *data();
  size_t size() const;

protected:
  using Resource::Resource;

private:
  friend ResourceManager;

  DataType m_type;
  IOBuffer m_lut;
};

}