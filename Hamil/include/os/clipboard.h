#pragma once

#include <common.h>

#include <os/error.h>

#include <string>

namespace os {

// Constrain lifetime as much as possible to prevent causing contention
class Clipboard {
public:
  struct OpenError : public os::Error {
    OpenError() : os::Error("OpenError") { }
  };

  struct GetSetError : public Error {
    GetSetError() : os::Error("GetSetError") { }
  };

  Clipboard() = default;
  Clipboard(const Clipboard& other) = delete;
  virtual ~Clipboard() = default;

  virtual std::string string() = 0;
  virtual void string(const void *ptr, size_t sz) = 0;
  void string(const std::string& str);
};


}
