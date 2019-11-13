#pragma once

#include <os/window.h>
#include <os/input.h>
#include <os/inputman.h>
#include <win32/thread.h>
#include <win32/glcontext.h>

#include <util/ref.h>
#include <math/geometry.h>

#include <config>

#include <utility>
#include <functional>
#include <map>

#if __win32
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

namespace gx {
// Forward declaration
class GLContext;
}

namespace win32 {

class Window final : public os::Window {
public:
  // See os::Window() for notes on 'width' and 'height'
  Window(int width, int height);
  virtual ~Window();

  // Passed to RegisterClass()
  static LPWSTR wnd_class_name() { return L"Hamil OpenGL"; }
  // Passed to CreateWindow()
  static LPWSTR wnd_name() { return L"Hamil"; }

  // Returns this Window's native handle
  HWND hwnd() const { return m_hwnd; }

  virtual void *nativeHandle() const final;

  virtual void swapBuffers() final;
  // TODO: glSwapInterval appears to only work once (?)
  virtual void swapInterval(unsigned interval) final;

  virtual void captureMouse() final;
  virtual void releaseMouse() final;
  virtual void resetMouse() final;

  virtual void quit() final;

  // Used to prevent double-delete because of for example
  //   calling os::finalize() before ~Window fires
  //  - See note above os::Window::destroy() for more
  //    details
  virtual void destroy() final;

protected:
  virtual bool doProcessMessages() final;

  virtual os::InputDeviceManager *acquireInputManager() final;

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

  // The thread which created the Window
  Thread::Id m_thread;
};

}
