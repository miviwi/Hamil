#pragma once

#include <win32/win32.h>
#include <win32/thread.h>

namespace win32 {

class Window;

// Acquired through Window::acquireGlContext()
class GlContext {
public:
  GlContext();
  GlContext(const GlContext& other) = delete;
  GlContext(GlContext&& other);

  GlContext& operator=(GlContext&& other);

  // Once called for the first time on a given Thread
  //   all future calls to this method must be made on
  //   that Thread
  void makeCurrent();

  // Frees the GlContext
  void release();

  // Returns 'true' if context was previously initialized
  operator bool();

private:
  friend Window;

  GlContext(void *hdc, void *hglrc);

  void /* HDC */ *m_hdc;
  void /* HGLRC */ *m_hglrc;

#if !defined(NDEBUG)
  Thread::Id m_owner = Thread::InvalidId;
#endif
};

}