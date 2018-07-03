#pragma once

#include <common.h>

#include <util/ref.h>

namespace win32 {

class Handle : public Ref {
public:
  Handle();
  Handle(void *handle);
  Handle(const Handle& other) = default;
  Handle(Handle&& other);
  ~Handle();

  Handle& operator=(const Handle& other) = delete;
  Handle& operator=(Handle&& other);

  void *handle() const;

protected:
  void *m;
};

}