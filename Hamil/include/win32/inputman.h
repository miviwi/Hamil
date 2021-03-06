#pragma once

#include <os/input.h>
#include <os/inputman.h>

namespace win32 {

using Input = os::Input;

using Mouse = os::Mouse;
using Keyboard = os::Keyboard;

// PIMPL struct
struct InputDeviceManagerData;

class InputDeviceManager final : public os::InputDeviceManager {
public:
  InputDeviceManager();
  virtual ~InputDeviceManager();

protected:
  virtual Input::Ptr doPollInput() final;

private:
  InputDeviceManagerData *m_data;
};

}
