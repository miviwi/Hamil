#pragma once

#include <gx/context.h>
#include <sysv/sysv.h>

namespace os {
// Forward declaration
class Window;
}

namespace sysv {

// Forward declaration
class Window;

// PIMPL struct
struct GLContextData;

class GLContext final : public gx::GLContext {
public:


  GLContext();
  virtual ~GLContext() final;

  virtual void *nativeHandle() const final;

  virtual gx::GLContext& acquire(os::Window *window, gx::GLContext *share = nullptr) final;

protected:
  virtual bool wasInit() const final;

  virtual void doMakeCurrent() final;
  virtual void doRelease() final;

private:
  friend Window;

  void swapBuffers();
  void swapInterval(unsigned interval);

  GLContextData *p;

  bool m_was_acquired;
};

}
