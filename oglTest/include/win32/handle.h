#pragma once

#include <common.h>

#include <util/ref.h>

namespace win32 {

class Handle : public Ref {
public:
  Handle();
  Handle(void *handle);
  Handle(Handle&& other);
  ~Handle();

  Handle& operator=(Handle&& other);

  void *handle() const;

protected:
  void *m;
};

}