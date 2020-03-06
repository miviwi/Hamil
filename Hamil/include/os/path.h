#pragma once

#include <common.h>

#include <string>

namespace os {

class Path {
public:
  Path();
  Path(const std::string& path);
  Path(const char *path);

  const std::string& path() const;

  // Returns everything before the last '/'
  std::string enclosingDir() const;

  // Returns everything after the last '/' (or after
  //   the second to last one in case there is
  //   a leading one)
  std::string back() const;

  Path& join(const Path& other);
  Path& join(const std::string& path);
  Path& join(const char *path);

private:
  std::string m;
};

}
