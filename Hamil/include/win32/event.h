#pragma once

#include <common.h>

#include <util/ref.h>
#include <win32/waitable.h>

namespace win32 {

class Event : public Waitable {
public:
  enum Flags {
    ManualReset = 1<<0,
  };

  struct Error {
    const unsigned what;
    Error(unsigned what_) :
      what(what_)
    { }
  };

  struct CreateError : public Error {
    using Error::Error;
  };

  struct SetResetError : public Error {
    using Error::Error;
  };

  // The 'state' parameter correspnds to the Event's initial signaled state -
  //   true - signaled, false - NOT signaled
  Event(bool state, unsigned flags = 0, const char *name = nullptr);

  void state(bool s) { s ? set() : reset(); }

  void set();
  void reset();
};

}