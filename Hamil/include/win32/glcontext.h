#pragma once

#include <win32/win32.h>
#include <win32/thread.h>

namespace win32 {

class Window;

class OGLContext {
public:
  void makeCurrent();

private:
  friend Window;

  OGLContext(void *hdc, void *hglrc);

  void * /* HDC */ m_hdc;
  void * /* HGLRC */ m_hglrc;

#if !defined(NDEBUG)
  Thread::Id m_owner = Thread::InvalidId;
#endif
};

}