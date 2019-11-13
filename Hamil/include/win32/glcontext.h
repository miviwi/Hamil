#pragma once

#include <gx/context.h>
#include <os/thread.h>
#include <win32/win32.h>

namespace os {
// Forward declaration
class Window;
}

namespace win32 {

// Acquired through Window::acquireGLContext()
class GLContext final : public gx::GLContext {
public:
  GLContext();
  GLContext(GLContext&& other);
  virtual ~GLContext() final;

  GLContext& operator=(GLContext&& other);

  virtual void *nativeHandle() const final;

  virtual gx::GLContext& acquire(os::Window *window, gx::GLContext *share = nullptr) final;

protected:
  virtual bool wasInit() const final;

  virtual void doMakeCurrent() final;
  virtual void doRelease() final;

private:
  void /* HDC */ *m_hdc;
  void /* HGLRC */ *m_hglrc;

#if !defined(NDEBUG)
  os::Thread::Id m_owner = os::Thread::InvalidId;
#endif
};

}
