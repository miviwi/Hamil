#include <os/window.h>
#include <os/inputman.h>

#include <cassert>
#include <cstdio>

namespace os {

Window::Window(int width, int height) :
  m_width(width), m_height(height),
  m_input_man(nullptr)
{
  assert((width >= 0) && (height >= 0) &&
      "attempted to create a Window with negative width/height!");
}

Input::Ptr Window::getInput()
{
  return m_input_man->pollInput();
}

void Window::mouseSpeed(float speed)
{
  m_input_man->mouseSpeed(speed);
}

bool Window::processMessages()
{
  return doProcessMessages();
}

void Window::quit()
{
  fprintf(stderr, "NOTE: os::Window::quit() unimplemented");
}

}
