#pragma once

#include <common.h>
#include <util/staticstring.h>

#include <string>
#include <memory>
#include <type_traits>

namespace res {

class Resource {
public:
  using Ptr = std::shared_ptr<Resource>;

  using Id = size_t;
  enum : Id { 
    InvalidId = ~0u,
  };

  using Tag = util::StaticString;

  enum Source {
    Memory, File, Archive,
  };

  Resource(Id id, Tag tag, const std::string& name, Source source, const std::string& path);
  virtual ~Resource() = default;

  template <typename T>
  static constexpr void is_resource()
  {
    static_assert(std::is_base_of_v<Resource, T>, "T must be derived from Resource!");
  }

  static constexpr Tag tag() { return "Resource"; }

  template <typename T>
  T *as()
  {
    is_resource<T>();
    return getTag() == T::tag() ? (T *)this : nullptr;
  }

  Id id() const { return m_id; }

  Tag getTag() const { return m_tag; }

  const std::string& name() const { return m_name; }

  Source source() const { return m_source; }
  const std::string& path() const { return m_path; }

  bool loaded() const { return m_loaded; }
  operator bool() const { return loaded(); }

  struct Hash    { size_t operator()(const Resource::Ptr& r) const { return r->id(); } };
  // Resources are considered the same when:
  //   - their types (tags) match
  //   - their names and paths match
  struct Compare { bool operator()(const Resource::Ptr& a, const Resource::Ptr& b) const; };

protected:
  bool m_loaded;

private:
  friend class ResourceManager; 

  Id m_id;       // hash generated by the loader from the tag, name and path

  Tag m_tag;
  std::string m_name;

  Source m_source;   // 'm_ource' does not contribute to the Resource's identity
                     // i.e. two Resources of the same type, name and path
                     // are considered equal even if one's loaded from a file
                     // and the other from an archive
  std::string m_path;
};

}