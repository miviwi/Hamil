#pragma once

#include <os/input.h>

#include <util/ref.h>
#include <math/geometry.h>

namespace gx {
// Forward declaration
class GLContext;
}

namespace os {

class InputDeviceManager;

class Window : public Ref {
public:
  // 'width' and 'height' will be the size of the
  //   so called (in Windows parlenance) "client area"
  //   i.e. the size of the framebuffer, not including
  //   the window decorations
  Window(int width, int height);
  virtual ~Window() = default;

  virtual void *nativeHandle() const = 0;

  // Process I/O messages from the OSes event loop
  //   - Must be called once per frame 
  //   - A return value of 'false' signifies the
  //     application should terminate
  bool processMessages();

  // Waits for VSync and swaps the front buffer with
  //   the back buffer
  virtual void swapBuffers() = 0;
  virtual void swapInterval(unsigned interval) = 0;

  // Takes uncomsumed input from the input buffer if
  //   there is any and returns it
  //  - Returns 'nullptr' in case there aren't any
  //    buffered events
  Input::Ptr getInput();
  // Calls InputDeviceManager::mouseSpeed() which
  //   sets 'speed' as the internal mouse cursor velocity
  //   multiplier (so a speed < 1 slows down the mouse
  //   and anything > 1 speeds it up)
  void mouseSpeed(float speed);
  
  // Hides the mouse cursor and locks it to the center of the window
  virtual void captureMouse() = 0;
  // Shows the mouse cursor and frees it
  virtual void releaseMouse() = 0;
  // Sets the mouse cursor position to the center of the window
  virtual void resetMouse() = 0;

  // Posts a QUIT message, which causes processMessages() to return 'false'
  virtual void quit() = 0;

  // Must be called on the thread which created the Window!
  //virtual gx::GLContext *acquireGLContext() = 0;

  // Must be called if ~Window() can fire after os::finalize()
  //   i.e. if any Windows exist in main() or are global variables
  //   they must have this method called on them
  //  - After calling this method the Window object becomes INVALID
  //    and any further operations on it result in undefined behaviour
  virtual void destroy() = 0;

protected:
  virtual bool doProcessMessages() = 0;

  // Must return a pointer to an implementation (derived
  //   class) of os::InputDeviceManager()
  virtual InputDeviceManager *acquireInputManager() = 0;

  int m_width, m_height;

private:
  InputDeviceManager *m_input_man;
};


}
