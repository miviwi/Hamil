#pragma once

#include <common.h>

#include <string>

namespace win32 {

// Constrain lifetime as much as possible to prevent causing contention
class Clipboard {
public:
  struct Error {
    virtual const char *what() const = 0;
  };

  struct OpenError : public Error {
    virtual const char *what() const { return "OpenError"; }
  };

  struct GetSetError : public Error {
    virtual const char *what() const { return "GetSetError"; }
  };

  Clipboard();
  Clipboard(const Clipboard& other) = delete;
  ~Clipboard();

  std::string string();
  void string(const void *ptr, size_t sz);
  void string(const std::string& str);
};

}