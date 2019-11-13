#pragma once

#include <os/input.h>
#include <os/inputman.h>

#include <util/ringbuffer.h>

namespace win32 {

using Input = os::Input;

using Mouse = os::Mouse;
using Keyboard = os::Keyboard;

class InputDeviceManager final : public os::InputDeviceManager {
public:
  InputDeviceManager();
  virtual ~InputDeviceManager() = default;

protected:
  virtual Input::Ptr doPollInput() final;

private:
  void doDoubleClick(Mouse *mi);

  RingBuffer<Mouse> m_clicks;
};

}
