#pragma once

#include <os/window.h>
#include <os/input.h>
#include <os/inputman.h>
#include <math/geometry.h>

namespace gx {
// Forward declaration
class GLContext;
}

namespace sysv {

// Forward declaration
class GLContext;

// PIMPL struct
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
  friend GLContext;

  // Used when creating a GLContext
  bool recreateWithVisual(u8 depth, u32 /* xcb_visualid_t */ visual);

  ivec2 m_dimensions;

  X11Window *p;
};

}
