#include <os/path.h>

#include <string>

#include <cassert>

namespace os {

using namespace std::literals::string_literals;

Path::Path()
{
}

Path::Path(const std::string& path) :
  m(path)
{
}

Path::Path(const char *path) :
  m(path)
{
}

const std::string& Path::path() const
{
  return m;
}

std::string Path::enclosingDir() const
{
  if(m.empty()) return "./";

  auto separator_pos = m.rfind('/');
  if(separator_pos == std::string::npos) return "./";

  return m.substr(0, separator_pos);
}

std::string Path::back() const
{
  if(m.empty()) return "";

  auto separator_pos = std::string::npos;
  if(m.back() == '/') {
    separator_pos = m.rfind('/', m.size()-2 /* start at the 2nd to last character */);
  } else {
    separator_pos = m.rfind('/');
  }

  if(separator_pos == std::string::npos) return m;

  return m.substr(separator_pos + /* skip the '/' */ 1);
}

Path& Path::join(const Path& other)
{
  return join(other.m);
}

Path& Path::join(const std::string& path)
{
  if(m.empty()) {
    m = path;

    return *this;
  }

  if(m.back() != '/') m += '/';

  m += path;

  return *this;
}

Path& Path::join(const char *path)
{
  return join(std::string(path));
}

}
