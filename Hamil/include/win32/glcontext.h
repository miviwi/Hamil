#pragma once

#include <win32/win32.h>
#include <win32/thread.h>

namespace win32 {

class Window;

// Acquired through Window::acquireOGLContext()
class OGLContext {
public:
  OGLContext();
  OGLContext(const OGLContext& other) = delete;
  OGLContext(OGLContext&& other);

  OGLContext& operator=(OGLContext&& other);

  // Once called for the first time on a given Thread
  //   all future calls to this method must be made on
  //   that Thread
  void makeCurrent();

  // Frees the OGLContext
  void release();

  // Returns 'true' if context was previously initialized
  operator bool();

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