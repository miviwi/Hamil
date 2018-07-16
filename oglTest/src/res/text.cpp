#include <res/text.h>

#include <cstring>

namespace res {

Resource::Ptr Text::from_memory(const char *buf, size_t sz, Id id, bool is_static,
   const std::string& name, const std::string& path)
{
  auto text = new Text(id, Text::tag(), name, Memory, path);

  if(is_static) {
    text->m_str = std::string_view(buf, sz);
  } else {
    text->fillBuffer(buf, sz);
  }
  text->m_loaded = true;

  return Resource::Ptr(text);
}

Resource::Ptr Text::from_file(const char *buf, size_t sz,
  Id id, const std::string& name, const std::string& path)
{
  auto text = new Text(id, Text::tag(), name, File, path);

  text->fillBuffer(buf, sz);
  text->m_loaded = true;

  return Resource::Ptr(text);
}

const std::string_view& Text::str() const
{
  return m_str;
}

const std::string_view *Text::operator->() const
{
  return &m_str;
}

void Text::fillBuffer(const char *buf, size_t sz)
{
  m_buf = std::make_unique<char[]>(sz + 1); // for NULL-byte
  m_str = std::string_view(m_buf.get(), sz);

  memcpy(m_buf.get(), buf, sz);
  m_buf[sz] = '\0'; // just to be safe...
}

}