#pragma once

#include <gx/context.h>
#include <sysv/sysv.h>

namespace os {
// Forward declaration
class Window;
}

namespace sysv {

class Window;

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
};

}
