#include <os/inputman.h>

namespace os {

InputDeviceManager::InputDeviceManager() :
  m_mouse_buttons(0),
  m_kb_modifiers(0), m_capslock(0),
  m_mouse_speed(1.0f), m_dbl_click_seconds(0.25f)
{
}

InputDeviceManager& InputDeviceManager::mouseSpeed(float speed)
{
  m_mouse_speed = speed;

  return *this;
}

float InputDeviceManager::mouseSpeed() const
{
  return m_mouse_speed;
}

InputDeviceManager& InputDeviceManager::doubleClickSpeed(float seconds)
{
  // TODO: convert 'seconds' to 'Time' and store it in a memebr variable

  m_dbl_click_seconds = seconds;    // Stored so the doubleClickSpeed()
                                    //   getter can later return it
                                    //   without doing any conversions

  return *this;
}

float InputDeviceManager::doubleClickSpeed() const
{
  return m_dbl_click_seconds;
}

Time InputDeviceManager::tDoubleClickSpeed() const
{
  fprintf(stderr, "NOTE: os::InputDeviceManager::tDoubleClickSpeed() unimplemented");

  return (Time)~0;
}

Input::Ptr InputDeviceManager::pollInput()
{
  Input::Ptr ptr(nullptr, &Input::deleter);

  // Poll for any new/latent Input structures
  //   and store them in an internal buffer...
  while(auto input = doPollInput()) {
    m_input_buf.push_back(std::move(input));
  }

  // ...and return one if any were stored
  if(m_input_buf.size()) {
    ptr = std::move(m_input_buf.front());
    m_input_buf.pop_front();
  }

  return ptr;
}

}
