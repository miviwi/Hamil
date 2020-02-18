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

void Window::initInput()
{
  m_input_man = acquireInputManager();
}

Input::Ptr Window::getInput()
{
  assert(m_input_man &&
      "Windw::getInput() can be called ONLY after Window::initInput() succeedes!");

  return m_input_man->pollInput();
}

void Window::mouseSpeed(float speed)
{
  assert(m_input_man &&
      "Windw::mouseSpeed() can be called ONLY after Window::initInput() succeedes!");

  m_input_man->mouseSpeed(speed);
}

bool Window::processMessages()
{
  return doProcessMessages();
}

}
