#pragma once

#include <os/window.h>
#include <os/input.h>
#include <os/inputman.h>

namespace gx {
// forward declaration
class GLContext;
}

namespace sysv {

// PIMPL structs
struct X11Connection;
struct X11Window;

class Window final : public os::Window {
public:
  // See note above os::Window()
  Window(int width, int height);
  virtual ~Window();

  virtual void *nativeHandle() const final;

  virtual void swapBuffers() final;
  virtual void swapInterval(unsigned interval) final;

  virtual void captureMouse() final;
  virtual void releaseMouse() final;
  virtual void resetMouse() final;

  virtual void quit() final;
  virtual void destroy() final;

protected:
  virtual bool doProcessMessages() final;

  virtual os::InputDeviceManager *acquireInputManager() final;

private:
  static X11Connection *p_x11;

  X11Window *p;
};

}
