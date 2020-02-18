#pragma once

#include <os/input.h>
#include <os/inputman.h>
#include <util/ringbuffer.h>

namespace sysv {

using Input = os::Input;

using Mouse = os::Mouse;
using Keyboard = os::Keyboard;

// PIMPL structs
struct InputDeviceManagerData;

struct LibevdevDevice;
struct LibevdevEvent;

class InputDeviceManager final : public os::InputDeviceManager {
public:
  InputDeviceManager();
  virtual ~InputDeviceManager();

protected:
  virtual Input::Ptr doPollInput() final;

private:
  InputDeviceManagerData *m_data;

  Input::Ptr keyboardEvent(const LibevdevEvent& libevdev_ev);
  Input::Ptr mouseEvent(const LibevdevEvent& libevdev_ev);
};

}
