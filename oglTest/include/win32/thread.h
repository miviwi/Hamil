#pragma once

#include <common.h>

#include <win32/waitable.h>

#include <functional>

namespace win32 {

class Thread : public Waitable {
public:
  using Id = ulong;
  using Fn = std::function<ulong()>;

  struct Error {
    const unsigned what;
    Error(unsigned what_) :
      what(what_)
    { }
  };

  struct CreateError : public Error {
    using Error::Error;
  };

  enum : ulong {
    StillActive = /* STILL_ACTIVE */ 259 ,
  };

  Thread(Fn fn, bool suspended = false);

  Id id() const;

  ulong exitCode() const;

private:
  static ulong ThreadProcTrampoline(void *param);

  Id m_id;
  Fn m_fn;
};

}