#pragma once

#include <res/resource.h>

#include <memory>
#include <string_view>

namespace res {

class Text : public Resource {
public:
  static constexpr Tag tag() { return "text"; }

  static Resource::Ptr from_memory(const char *buf, size_t sz, Id id,     // 'buf' is copied only when !is_static
    bool is_static = false, const std::string& name = "", const std::string& path = "");

  static Resource::Ptr from_file(const char *buf, size_t sz, Id id,       // 'buf' is always copied
    const std::string& name = "", const std::string& path = "");

  const std::string_view& str() const;
  const std::string_view *operator->() const; // accesses str()

protected:
  using Resource::Resource;

private:
  friend class ResourceManager; 

  // memcpys 'buf' to 'm_buf' and sets 'm_str'
  void fillBuffer(const char *buf, size_t sz);

  std::string_view m_str;
  std::unique_ptr<char[]> m_buf;
};

}