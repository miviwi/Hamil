#include <res/resource.h>

namespace res {

Resource::Resource(Id id, Tag tag, const std::string& name, Source source, const std::string& path) :
  m_loaded(false), m_id(id), m_tag(tag), m_name(name), m_source(source), m_path(path)
{
}

bool Resource::Compare::operator()(const Resource::Ptr& a, const Resource::Ptr& b) const
{
  return a->getTag() == b->getTag() && a->name() == b->name() && a->path() == b->path();
}

}