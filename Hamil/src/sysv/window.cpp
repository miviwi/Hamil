#include <sysv/window.h>
#include <sysv/inputman.h>
#include <gx/context.h>

namespace sysv {

Window::Window(int width, int height) :
  os::Window(width, height)
{
}

Window::~Window()
{
}

void *Window::nativeHandle() const
{
  return nullptr;
}

void Window::swapBuffers()
{
}

void Window::swapInterval(unsigned interval)
{
}

void Window::captureMouse()
{
}

void Window::releaseMouse()
{
}

void Window::resetMouse()
{
}

void Window::quit()
{
}

void Window::destroy()
{
}

bool Window::doProcessMessages()
{
#if __sysv
  // TODO: implement :)

  return false;
#else
  return false;
#endif
}

os::InputDeviceManager *Window::acquireInputManager()
{
  return new sysv::InputDeviceManager();
}

}
