#pragma once

#include <os/input.h>
#include <util/ringbuffer.h>

#include <deque>

namespace os {

class InputDeviceManager {
public:
  InputDeviceManager();
  virtual ~InputDeviceManager() = default;

  InputDeviceManager& mouseSpeed(float speed);
  float mouseSpeed() const;

  InputDeviceManager& doubleClickSpeed(float seconds);
  float doubleClickSpeed() const;

  // Can return nullptr in case no input events have occured
  Input::Ptr pollInput();

protected:
  // Equivalent to doubleClickSpeed(), except the
  //   return value is of type 'Time' (a.k.a in
  //   units of ticks, not seconds)
  Time tDoubleClickSpeed() const;

  // Via an internal buffer and max possible double click
  //   delay determined by tDoubleClickSpeed() determine
  //   if 'mi' corresponds to a Mouse::DoubleClick event
  //   and mutate it if so
  void doDoubleClick(Mouse *mi);

  // Should return a nullptr immediately
  //   if there's no input available from
  //   the OS and NOT block
  virtual Input::Ptr doPollInput() = 0;

  unsigned m_mouse_buttons;
  unsigned m_kb_modifiers;
  unsigned m_capslock;

private:
  float m_mouse_speed;
  float m_dbl_click_seconds;

  std::deque<Input::Ptr> m_input_buf;

  RingBuffer<Mouse> m_clicks;
};

}
