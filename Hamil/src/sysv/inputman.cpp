#include <sysv/inputman.h>

#include <cassert>

// X11/xcb headers
#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/keysymdef.h>

namespace sysv {

struct InputDeviceManagerData {
  xcb_connection_t *connection;
};

InputDeviceManager::InputDeviceManager() :
  m_data(nullptr)
{
}

Input::Ptr InputDeviceManager::doPollInput()
{
  Input::Ptr input(nullptr, &Input::deleter);

#if __sysv
#endif

  return input;
}

}
