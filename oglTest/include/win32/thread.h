#pragma once

#include <common.h>

#include <functional>

namespace win32 {

class Thread {
public:
  using Id = ulong;
  using Fn = std::function<ulong()>;

  struct Error { };
  struct CreateError : public Error { };

  enum : ulong {
    StillActive = /* STILL_ACTIVE */ 259 ,
  };

  Thread(Fn fn, bool suspended = false);
  ~Thread();

  Id id() const;

  ulong exitCode() const;

private:
  static ulong ThreadProcTrampoline(void *param);

  void *m_handle;
  Id m_id;
  Fn m_fn;
};

}