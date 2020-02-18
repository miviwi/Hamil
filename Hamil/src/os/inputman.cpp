#include <os/inputman.h>
#include <os/time.h>
#include <math/geometry.h>

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

void InputDeviceManager::doDoubleClick(Mouse *mi)
{
  // In case of some mose movenet during the clicks, check
  //   if said movement doesn't exceed a set fudge factor
  if(mi->event != Mouse::Down) {
    auto d = vec2(mi->dx, mi->dy);
    if(d.length2() > 5.0f*5.0f) m_clicks.clear();   // And if it does make sure the next click
                                                    //   won't coun't as a double click
    return;
  }

  if(!m_clicks.empty()) {  // This is the second Mouse::Down input of the double click...
    auto last_click = m_clicks.last();
    auto dt = mi->timestamp - last_click.timestamp;

    m_clicks.clear();

    // Ensure the clicks happened below the threshold
    if(last_click.ev_data == mi->ev_data && dt < tDoubleClickSpeed()) {
      mi->event = Mouse::DoubleClick;
      return;
    }
  }

  // ...and this is the first
  m_clicks.push(*mi);
}

Time InputDeviceManager::tDoubleClickSpeed() const
{
  return Timers::s_to_ticks(m_dbl_click_seconds);
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
