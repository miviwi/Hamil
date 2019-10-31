#pragma once

#include <win32/input.h>
#include <win32/thread.h>
#include <win32/glcontext.h>

#include <util/ref.h>
#include <math/geometry.h>

#include <utility>
#include <functional>
#include <map>

#if defined(_MSVC_VER)
#  include <Windows.h>
#else
#  define LPWSTR wchar_t*
#  define HINSTANCE void*
#  define HWND void*
#  define HGLRC void*
#  define LRESULT int
#  define LPARAM int
#  define WPARAM int
#  define UINT unsigned
#  define ATOM unsigned short
#  define CALLBACK
#endif

namespace win32 {

class Window : public Ref {
public:
  // 'width' and 'height' will be the size of the
  //   "client (drawable) area" i.e. of the framebuffer
  Window(int width, int height);
  ~Window();

  // Passed to RegisterClass()
  static LPWSTR wnd_class_name() { return L"Hamil OpenGL"; }
  // Passed to CreateWindow()
  static LPWSTR wnd_name() { return L"Hamil"; }

  // Returns this Window's native handle
  HWND hwnd() const { return m_hwnd; }

  // Returns 'false' when a QUIT message was posted and the
  //   application should terminate
  //  - Fills the input buffer and repaints the window
  bool processMessages();

  // Waits for VSync and swaps the front buffer with
  //   the back buffer
  void swapBuffers();
  // TODO: glSwapInterval appears to only work once (?)
  void swapInterval(unsigned interval);

  // Takes uncomsumed input from the input buffer if
  //   there is any and returns it
  Input::Ptr getInput();
  // Calls InputDeviceManager::setMouseSpeed() which
  //   sets 'speed' as the internal mouse cursor velocity
  //   multiplier (so a speed < 1 slows down the mouse
  //   and anything > 1 speeds it up)
  void setMouseSpeed(float speed);

  // Hides the mouse cursor and locks it to the center of the window
  void captureMouse();
  // Shows the mouse cursor and frees it
  void releaseMouse();
  // Sets the mouse cursor position to the center of the window
  void resetMouse();

  // Posts a QUIT message (causes processMessages() to return false)
  void quit();

  // Must be called on the thread which created the Window!
  GlContext acquireGlContext();

  // Must be called if ~Window() can fire after win32::finalize()
  //   i.e. if any Windows exist in main() or are global variables
  //   they must have this method called on them
  //  - After calling this method the Window object becomes INVALID
  //    and any further operations on it result in undefined behaviour
  void destroy();

private:
  // Declared as member so private fields can be accessed inside
  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam);

  // Registers the class only if this method has never been called
  //   before and returns it's ATOM
  ATOM registerClass(HINSTANCE hInstance);
  // Calls CreateWindow()
  HWND createWindow(HINSTANCE hInstance, int width, int height);

  HWND m_hwnd;
  // Main OpenGL context
  //   - Created by the constructor
  //   - Used as the 'hShareContext' for contexts created
  //     via Window::acquireGlContext()
  HGLRC m_hglrc;
  // Dimensions of the client area/framebuffer
  int m_width, m_height;

  // The thread which created the Window
  Thread::Id m_thread;

  InputDeviceManager m_input_man;
};

}
