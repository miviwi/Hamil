#pragma once

#include <os/clipboard.h>

#include <string>

namespace sysv {

class Clipboard final : public os::Clipboard {
public:
  Clipboard();
  virtual ~Clipboard();

  virtual std::string string() final;
  virtual void string(const void *ptr, size_t sz) final;
};

}
