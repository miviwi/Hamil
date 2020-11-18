#include "os/time.h"
#include <cstdio>
#include <sysv/inputman.h>
#include <sysv/sysv.h>
#include <sysv/time.h>
#include <sysv/x11.h>
#include <os/input.h>
#include <os/panic.h>
#include <math/geometry.h>
#include <math/util.h>
#include <util/format.h>

#include <config>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <cctype>

#include <array>
#include <algorithm>
#include <optional>

// POSIX headers
#if __sysv
#  include <unistd.h>
#  include <fcntl.h>
#  include <glob.h>
#  include <sys/stat.h>
#endif

// X11/xcb headers
#if __sysv
#  include <xcb/xcb.h>
#  include <X11/Xlib.h>
#  include <X11/Xlib-xcb.h>
#  include <X11/keysymdef.h>
#endif

// libevdev
#if __sysv
#  include <linux/input.h>
#  include <linux/input-event-codes.h>
#  include <libevdev/libevdev.h>
#endif

// Uncomment the following line to disable mapping
//   input_evennt::type == EV_ABS events -> os::Mouse inputs
//  - FOR DEBUG PURPOSES ONLY!
//#define LIBEVDEV_NO_TOUCHPAD_AS_MOUSE 1

namespace sysv {

using x11_detail::x11;

enum DeviceType {
  DeviceInvalid,
  DeviceMouse,
  DeviceKeyboard,
};

static const char *DeviceType_to_path[] = {
  "",                    // DeviceInvalid
  "mouse",               // DeviceMouse
  "kbd",                 // DeviceKeyboard
};

struct LibevdevEnumDevices {
  static LibevdevEnumDevices enum_by_type(DeviceType type);

  operator bool() const { return m_glob.has_value(); }

  size_t numDevices() const { return m_glob->gl_pathc; }
  const char *path(size_t idx) const
  {
    assert(idx < numDevices());

    return m_glob->gl_pathv[idx];
  }

  ~LibevdevEnumDevices()
  {
    if(!m_glob) return;
    
    globfree(globPtr());
  }

private:
  LibevdevEnumDevices() = default;

  glob_t *globPtr() { return m_glob.has_value() ? &m_glob.value() : nullptr; }

  std::optional<glob_t> m_glob = std::nullopt;
};

struct LibevdevDevice {
#if __sysv
  int fd = -1;
  struct libevdev *device = nullptr;
#endif
};

struct LibevdevEvent {
#if __sysv
  struct input_event ev;
#endif
};

enum LibevdevKeyboardEventValue {
  KeyboardEventKeyDown = 1,
  KeyboardEventKeyUp   = 0,

  KeyboardEventKeyHeld = 2,

  MouseEventButtonDown = 1,
  MouseEventButtonUp   = 0,
};

struct KeyboardKeyState {
  bool down = false;
  os::Time pressed_timestamp = os::InvalidTime;
};

struct InputDeviceManagerData {
#if __sysv
  xcb_connection_t *connection = nullptr;

  LibevdevDevice mouse;
  LibevdevDevice kb;

  bool mouse_is_touchpad = false;
  bool mouse_has_hi_res_wheel = false;

  bool touchpad_needs_recenter = true;

  ivec4 touchpad_extents = ivec4::zero();    // (x, y, z, w) = (min_x, min_y, max_x, max_y)
  ivec2 touchpad_last_pos = ivec2(-1, -1);

  std::array<KeyboardKeyState, 256> kb_keys;

  struct libevdev *dev_mouse() { return mouse.device; }
  struct libevdev *dev_kb()    { return kb.device; }
#endif
};

LibevdevEnumDevices LibevdevEnumDevices::enum_by_type(DeviceType type)
{
  LibevdevEnumDevices devices;
  devices.m_glob.emplace();     // Init the glob_t

  auto glob_pattern = util::fmt("/dev/input/by-path/*-event-%s", DeviceType_to_path[type]);

  int glob_err = glob(glob_pattern.data(), 0, nullptr, devices.globPtr());
  if(glob_err) {
    devices.m_glob = std::nullopt;    // If the glob() call fails enuse globfree()
  }                                   //   doesn't get called

  return devices;
} 

static LibevdevDevice open_device(const LibevdevEnumDevices& devices, size_t which)
{
  assert(devices && "invalid LibevdevEnumDevices passed to open_device()!");

  LibevdevDevice device;
  
  auto path = devices.path(which);

  int fd_device = open(path, O_RDONLY | O_NONBLOCK);
  if(fd_device < 0) {
    os::panic(
        util::fmt("failed to open `%s'!", path).data(),
        os::LibevdevDeviceOpenError
    );
  }

  struct libevdev *pdevice = nullptr;
  int device_init_err = libevdev_new_from_fd(fd_device, &pdevice);
  if(device_init_err) {
    os::panic(
        util::fmt("libevdev_new_from_fd(\"%s\") failed!", path).data(),
        os::LibevdevDeviceOpenError
    );
  }

  device.fd = fd_device;
  device.device = pdevice;

  return device;
}

static void close_device(const LibevdevDevice& device)
{
  libevdev_free(device.device);
  close(device.fd);
}

static bool filtered_event(const LibevdevEvent& libevdev_ev);

static std::optional<LibevdevEvent> next_event(const LibevdevDevice& libevdev_device)
{
  LibevdevEvent libevdev_ev;

  auto device = libevdev_device.device;

  int next_event_err = libevdev_next_event(
      device, LIBEVDEV_READ_FLAG_NORMAL,
      &libevdev_ev.ev
  );
  if(!next_event_err && !filtered_event(libevdev_ev)) {
#if 0
    const char *device_name = libevdev_get_name(device);
    const auto& ev = libevdev_ev.ev;

    int value = libevdev_get_event_value(device, ev.type, ev.code);

    if(ev.type == EV_KEY) {
      printf("'%s' %s event (%s=%d)\n",
          device_name,
          libevdev_event_type_get_name(ev.type),
          libevdev_event_code_get_name(ev.type, ev.code), ev.value
      );
    } else if((ev.type == EV_ABS || ev.type == EV_REL) && false) {
      auto abs = libevdev_get_abs_info(device, ev.code);

      printf("'%s' %s event (%s=%d)",
          device_name,
          libevdev_event_type_get_name(ev.type),
          libevdev_event_code_get_name(ev.type, ev.code), ev.value
      );

      if(abs) {
        printf("abs.value=%d", abs->value);
      }

      printf("\n");
    }
#endif
  } else if(next_event_err && next_event_err != -EAGAIN) {
#if !defined(NDEBUG)
    printf("libevdev_next_event() error %s!\n", strerror(-next_event_err));
#endif

    return std::nullopt;
  } else if(next_event_err == -EAGAIN) {
    return std::nullopt;   // No more events to process currently
  }

  return libevdev_ev;
}

// Returns 'true' if the passed in event should be ignored
static bool filtered_event(const LibevdevEvent& libevdev_ev)
{
  // XXX: Use libevdev_disable_event_code() here to filter EV_MSC events instead?

  const auto& ev = libevdev_ev.ev;

  switch(ev.type) {
  case EV_SYN:                      // Ignore Synchronization events...
  case EV_MSC: return true;        //   ...and Miscellaneous events as well
  }

  // Filter out multitouch related events and touchpad-specific ones
  if(ev.type == EV_ABS) {
    switch(ev.code) {
    case ABS_MT_SLOT:
    case ABS_MT_BLOB_ID:
    case ABS_MT_TRACKING_ID:

    case ABS_MT_POSITION_X:
    case ABS_MT_POSITION_Y:

    case ABS_PRESSURE:
    case ABS_MT_PRESSURE:
    case ABS_MT_DISTANCE:
    case ABS_MT_ORIENTATION:

    case ABS_MT_TOOL_TYPE:

    case ABS_MT_TOOL_X:
    case ABS_MT_TOOL_Y: return true;
    }
  }

  return false;
}

static unsigned translate_sym(int code, unsigned modifiers);
static unsigned translate_key(int code, unsigned modifiers);

InputDeviceManager::InputDeviceManager() :
  m_data(nullptr)
{
#if __sysv
  m_data = new InputDeviceManagerData();

  auto mice_enum = LibevdevEnumDevices::enum_by_type(DeviceMouse);
  if(!mice_enum || !mice_enum.numDevices()) {
    os::panic("failed to find a mouse device!", os::LibevdevDeviceOpenError);
  }

  auto keyboard_enum = LibevdevEnumDevices::enum_by_type(DeviceKeyboard); 
  if(!keyboard_enum || !keyboard_enum.numDevices()) {
    os::panic("failed to find a keyboard device!", os::LibevdevDeviceOpenError);
  }

 // m_data->mouse = open_device(mice_enum, mice_enum.numDevices() > 1 ? 1 : 0 /* use touchpad if possible*/);
  m_data->mouse = open_device(mice_enum, 0);
  m_data->kb = open_device(keyboard_enum, 0);

  auto mouse_has_prop = [&](int type, int code) -> bool {
    return libevdev_has_event_code(m_data->dev_mouse(), type, code);
  };

  m_data->mouse_is_touchpad = mouse_has_prop(EV_ABS, ABS_X); // 'ABS_Y' would work just as well here
  m_data->mouse_has_hi_res_wheel = mouse_has_prop(EV_REL, REL_WHEEL_HI_RES);

#if !defined(LIBEVDEV_NO_TOCHPAD_AD_MP)
  if(m_data->mouse_is_touchpad) {
    os::panic(
        "sysv::InputDeviceManager: touchpad as mouse support unimplemented",
        os::LibevdevDeviceOpenError
    );
  }
#endif

#if 0 
  if(m_data->mouse_is_touchpad) {
     const struct input_absinfo *abs_x = libevdev_get_abs_info(m_data->dev_mouse(), ABS_X);
     const struct input_absinfo *abs_y = libevdev_get_abs_info(m_data->dev_mouse(), ABS_Y);

     assert(abs_x && abs_y);

     // TODO: is the extent data really needed?
     m_data->touchpad_extents = {
       abs_x->minimum, abs_y->minimum,
       abs_x->maximum, abs_y->maximum,
     };

     m_data->touchpad_last_pos = {
       abs_x->value, abs_y->value,
     };
  }
#endif

  m_capslock = 0;    // FIXME: query CapsLock's current state here!
#endif
}

InputDeviceManager::~InputDeviceManager()
{
#if __sysv
  close_device(m_data->mouse);
  close_device(m_data->kb);
#endif

  delete m_data;
}

Input::Ptr InputDeviceManager::doPollInput()
{
  auto input = Input::invalid_ptr();

#if __sysv
  // Poll the keyboard...
  while(std::optional<LibevdevEvent> opt_ev = next_event(m_data->kb)) {
    const auto& ev = opt_ev.value();
    if(filtered_event(ev)) continue;

    if(auto input = keyboardEvent(ev)) return input;
  }

  // ...and then the mouse
  while(std::optional<LibevdevEvent> opt_ev = next_event(m_data->mouse)) {
    const auto& ev = opt_ev.value();
    if(filtered_event(ev)) continue;

    // TODO: Merge series of EV_REL events into a single Mouse::Move event
    if(auto input = mouseEvent(ev)) return input;
  }

#endif

  return input;
}

Input::Ptr InputDeviceManager::keyboardEvent(const LibevdevEvent& libevdev_ev)
{
  const auto& ev = libevdev_ev.ev;
  auto timestamp = sysv::Timers::ticks();

  if(ev.value == KeyboardEventKeyHeld) return Input::invalid_ptr();

  auto input = Keyboard::create();
  input->timestamp = timestamp;

  auto kbi = (Keyboard *)input.get();

  u16 event = 0;
  switch(ev.value) {
  case KeyboardEventKeyUp:   event = Keyboard::KeyUp; break;
  case KeyboardEventKeyDown: event = Keyboard::KeyDown; break;

  default: assert(0);    // Unreachable
  }

  unsigned modifiers = 0;
  switch(ev.code) {
  case KEY_LEFTCTRL:
  case KEY_RIGHTCTRL:  modifiers = Keyboard::Ctrl; break;

  case KEY_LEFTSHIFT:
  case KEY_RIGHTSHIFT: modifiers = Keyboard::Shift; break;

  case KEY_LEFTALT:
  case KEY_RIGHTALT:   modifiers = Keyboard::Alt; break;

  case KEY_LEFTMETA:
  case KEY_RIGHTMETA:  modifiers = Keyboard::Super; break;
  }

  if(event == Keyboard::KeyUp) {
    m_kb_modifiers &= ~modifiers;
  } else if(event == Keyboard::KeyDown) {
    m_kb_modifiers |= modifiers;

    if(ev.code == KEY_CAPSLOCK) m_capslock ^= Keyboard::CapsLock; // Toggles between Capslock and 0
  }

  // Populate Keyboard::time_held
  //   - Update m_data->kb_keys as well
  auto& key_data = m_data->kb_keys.at(ev.code);
  if(event == Keyboard::KeyDown) {
    key_data.down = true;
    key_data.pressed_timestamp = timestamp;

    kbi->time_held = 0;
  } else if(event == Keyboard::KeyUp) {
    auto pressed_timestamp = key_data.pressed_timestamp;

    key_data.down = false;
    key_data.pressed_timestamp = os::InvalidTime;

    kbi->time_held = timestamp - pressed_timestamp;
  }

  kbi->event = event;
  kbi->modifiers = m_kb_modifiers | m_capslock;

  kbi->key = translate_key(ev.code, kbi->modifiers);
  kbi->sym = translate_sym(ev.code, kbi->modifiers);

  return input;    // Got an EV_KEY - so return from the loop
}

Input::Ptr InputDeviceManager::mouseEvent(const LibevdevEvent& libevdev_ev)
{
  const auto& ev = libevdev_ev.ev;

  if(m_data->mouse_has_hi_res_wheel) {
    // If REL_WHEEL_HI_RES events are available make sure to ignore REL_WHEEL
    //   so Mouse::Wheel inputs aren't doubled
    if(libevdev_ev.ev.type == EV_REL && libevdev_ev.ev.code == REL_WHEEL) return Input::invalid_ptr();
  }

  auto input = Mouse::create();
  input->timestamp = sysv::Timers::ticks();

  auto mi = (Mouse *)input.get();

  mi->dx = mi->dy = 0.0f;
  mi->ev_data = 0;

  u16 event = 0;
  if(ev.type == EV_KEY) {
    switch(ev.value) {
    case MouseEventButtonUp:   event = Mouse::Up; break;
    case MouseEventButtonDown: event = Mouse::Down; break;

    default: assert(0);    // Unreachable
    }

    // Populate button data
    u16 buttons = 0;
    if(!m_data->mouse_is_touchpad) {
      switch(ev.code) {
      case BTN_LEFT:   buttons = Mouse::Left; break;
      case BTN_RIGHT:  buttons = Mouse::Right; break;
      case BTN_MIDDLE: buttons = Mouse::Middle; break;
      case BTN_4:      buttons = Mouse::X1; break;
      case BTN_5:      buttons = Mouse::X2; break;
      }
    } else {
      switch(ev.code) {
      case BTN_LEFT:           buttons = Mouse::Left; break;
      case BTN_TOOL_DOUBLETAP: buttons = Mouse::Right; break;

      // Have to recenter the touch tracking pos on the net EV_ABS
      case BTN_TOUCH:
        if(ev.value == MouseEventButtonUp) {
          m_data->touchpad_needs_recenter = true;
          m_data->touchpad_last_pos = { -1, -1 };

        } 
        return Input::invalid_ptr();
      }
    }

    if(event == Mouse::Up) {
      m_mouse_buttons &= ~buttons;
    } else if(event == Mouse::Down) {
      m_mouse_buttons |= buttons;
    }

    mi->ev_data = buttons;
  } else if(ev.type == EV_REL) {
    switch(ev.code) {
    // Mouse
    case REL_X:
    case REL_Y: event = Mouse::Move; break;

    case REL_WHEEL:
    case REL_WHEEL_HI_RES: event = Mouse::Wheel; break;

    default: assert(0);     // Unreachable (?)
    }

    // Populate Mouse::dx, Mouse::dy (for REL_X,REL_Y)
    //
    // And Mouse::ev_data (for REL_WHEEL[_HI_RES]
    switch(ev.code) {
    case REL_X: mi->dx = (float)ev.value*mouseSpeed(); break;
    case REL_Y: mi->dy = (float)ev.value*mouseSpeed(); break;

    case REL_WHEEL_HI_RES: mi->ev_data = ev.value; break;

    // Filtered out above in case REL_WHEEL_HI_RES events are generated by this device
    case REL_WHEEL: mi->ev_data = ev.value * 120 /* map into REL_WHEEL_HI_RES rage */; break;

    default: assert(0);   // Unreachable
    }
  } else if(ev.type == EV_ABS) {
    switch(ev.code) {
    // Mouse
    case ABS_X:
    case ABS_Y: event = Mouse::Move; break;

    // TODO: translate scroll into Mouse::Wheel events here

    default: assert(0);     // Unreachable (?)
    }

    auto last_pos = vec2::inf();
    if(m_data->touchpad_needs_recenter) {    // Make the current { ABS_X, ABS_Y }
      auto touchpad_last_pos = m_data->touchpad_last_pos;
      switch(ev.code) {
      case ABS_X: m_data->touchpad_last_pos = { ev.value, touchpad_last_pos.y }; break;
      case ABS_Y: m_data->touchpad_last_pos = { touchpad_last_pos.x, ev.value }; break;
      }

      if(m_data->touchpad_last_pos.x < 0 || m_data->touchpad_last_pos.y < 0) {
        return Input::invalid_ptr();
      }

      printf("recendered touchpad relative move center\n"
          "     -> %s\n",

          math::to_str(m_data->touchpad_last_pos).data()
      );

      // ...and make sure we don't recenter again
      m_data->touchpad_needs_recenter = false;

      return Input::invalid_ptr();   // Squash the input
    } else {
      last_pos = m_data->touchpad_last_pos.cast<float>();
    }

    // Populate Mouse::dx, Mouse::dy (for ABS_X,ABS_Y)
    switch(ev.code) {
    case ABS_X: {
      mi->dx = (float)ev.value*mouseSpeed() - last_pos.x;

      // Update the refrence point
      m_data->touchpad_last_pos = {
        ev.value, m_data->touchpad_last_pos.y,
      };
      break;
    }

    case ABS_Y: {
      mi->dy = (float)ev.value*mouseSpeed() - last_pos.y;

      // Update the refrence point
      m_data->touchpad_last_pos = {
        m_data->touchpad_last_pos.x, ev.value,
      };
      break;
    }

    default: assert(0);    // Unreachable
    }
  }

  mi->event = event;
  mi->buttons = m_mouse_buttons;

  doDoubleClick(mi);

  return input;
}

// TODO: move this into an externally generated table?
static int translate_code(int code)
{
#if __sysv
  switch(code) {
  case KEY_1: return '1';
  case KEY_2: return '2';
  case KEY_3: return '3';
  case KEY_4: return '4';
  case KEY_5: return '5';
  case KEY_6: return '6';
  case KEY_7: return '7';
  case KEY_8: return '8';
  case KEY_9: return '9';
  case KEY_0: return '0';

  case KEY_Q: return 'Q';
  case KEY_W: return 'W';
  case KEY_E: return 'E';
  case KEY_R: return 'R';
  case KEY_T: return 'T';
  case KEY_Y: return 'Y';
  case KEY_U: return 'U';
  case KEY_I: return 'I';
  case KEY_O: return 'O';
  case KEY_P: return 'P';
  case KEY_A: return 'A';
  case KEY_S: return 'S';
  case KEY_D: return 'D';
  case KEY_F: return 'F';
  case KEY_G: return 'G';
  case KEY_H: return 'H';
  case KEY_J: return 'J';
  case KEY_K: return 'K';
  case KEY_L: return 'L';
  case KEY_Z: return 'Z';
  case KEY_X: return 'X';
  case KEY_C: return 'C';
  case KEY_V: return 'V';
  case KEY_B: return 'B';
  case KEY_N: return 'N';
  case KEY_M: return 'M';

  case KEY_GRAVE:      return '`';
  case KEY_MINUS:      return '-';
  case KEY_EQUAL:      return '=';
  case KEY_BACKSPACE:  return Keyboard::Backspace;
  case KEY_TAB:        return Keyboard::Tab;
  case KEY_ENTER:      return Keyboard::Enter;
  case KEY_LEFTBRACE:  return '[';
  case KEY_RIGHTBRACE: return ']';
  case KEY_SEMICOLON:  return ';';
  case KEY_APOSTROPHE: return '\'';
  case KEY_BACKSLASH:  return '\\';
  case KEY_COMMA:      return ',';
  case KEY_DOT:        return '.';
  case KEY_SLASH:      return '/';

  case KEY_ESC: return Keyboard::Escape;
  case KEY_F1:  return Keyboard::F1;
  case KEY_F2:  return Keyboard::F2;
  case KEY_F3:  return Keyboard::F3;
  case KEY_F4:  return Keyboard::F4;
  case KEY_F5:  return Keyboard::F5;
  case KEY_F6:  return Keyboard::F6;
  case KEY_F7:  return Keyboard::F7;
  case KEY_F8:  return Keyboard::F8;
  case KEY_F9:  return Keyboard::F9;
  case KEY_F10: return Keyboard::F10;
  case KEY_F11: return Keyboard::F11;
  case KEY_F12: return Keyboard::F12;

  case KEY_UP:    return Keyboard::Up;
  case KEY_LEFT:  return Keyboard::Left;
  case KEY_DOWN:  return Keyboard::Down;
  case KEY_RIGHT: return Keyboard::Right;

  case KEY_INSERT:   return Keyboard::Insert;
  case KEY_HOME:     return Keyboard::Home;
  case KEY_PAGEUP:   return Keyboard::PageUp;
  case KEY_DELETE:   return Keyboard::Delete;
  case KEY_END:      return Keyboard::End;
  case KEY_PAGEDOWN: return Keyboard::PageDown;

  case KEY_NUMLOCK:    return Keyboard::NumLock;
  case KEY_SYSRQ:      return Keyboard::Print;
  case KEY_SCROLLLOCK: return Keyboard::ScrollLock;
  case KEY_PAUSE:      return Keyboard::Pause;

  case KEY_LEFTCTRL:
  case KEY_RIGHTCTRL:  return Keyboard::SpecialKey | Keyboard::Ctrl;
  case KEY_LEFTSHIFT:
  case KEY_RIGHTSHIFT: return Keyboard::SpecialKey | Keyboard::Shift;
  case KEY_LEFTALT:
  case KEY_RIGHTALT:   return Keyboard::SpecialKey | Keyboard::Alt;
  case KEY_LEFTMETA:
  case KEY_RIGHTMETA:  return Keyboard::SpecialKey | Keyboard::Super;
  case KEY_CAPSLOCK:   return Keyboard::SpecialKey | Keyboard::CapsLock;

  default: return -1;
  }
#endif

  assert(0);    // Unreachable
  return -1;
}

static unsigned translate_sym(int code, unsigned modifiers)
{
  int ch = translate_code(code);

  bool shift = modifiers & Keyboard::Shift,
       caps = modifiers & Keyboard::CapsLock;

  // If CapsLock is currently ON Shift's function is inversed for letters
  bool upper = caps ? !shift : shift;

#if __sysv
  if (shift) {
    switch(code) {
    case KEY_1: return '!';
    case KEY_2: return '@';
    case KEY_3: return '#';
    case KEY_4: return '$';
    case KEY_5: return '%';
    case KEY_6: return '^';
    case KEY_7: return '&';
    case KEY_8: return '*';
    case KEY_9: return '(';
    case KEY_0: return ')';

    case KEY_SEMICOLON:  return ':';
    case KEY_SLASH:      return '?';
    case KEY_GRAVE:      return '~';
    case KEY_LEFTBRACE:  return '{';
    case KEY_BACKSLASH:  return '|';
    case KEY_RIGHTBRACE: return '}';
    case KEY_APOSTROPHE: return '\"';
    case KEY_COMMA:      return '<';
    case KEY_DOT:        return '>';
    case KEY_EQUAL:      return '+';
    case KEY_MINUS:      return '_';
    }
  } else {
    switch(code) {
    case KEY_1: return '1';
    case KEY_2: return '2';
    case KEY_3: return '3';
    case KEY_4: return '4';
    case KEY_5: return '5';
    case KEY_6: return '6';
    case KEY_7: return '7';
    case KEY_8: return '8';
    case KEY_9: return '9';
    case KEY_0: return '0';

    case KEY_SEMICOLON:  return ';';
    case KEY_SLASH:      return '/';
    case KEY_GRAVE:      return '`';
    case KEY_LEFTBRACE:  return '[';
    case KEY_BACKSLASH:  return '\\';
    case KEY_RIGHTBRACE: return ']';
    case KEY_APOSTROPHE: return '\'';
    case KEY_COMMA:      return ',';
    case KEY_DOT:        return '.';
    case KEY_EQUAL:      return '=';
    case KEY_MINUS:      return '-';
    }
  }
#endif

  return upper ? toupper(ch) : tolower(ch);
}

static unsigned translate_key(int code, unsigned modifiers)
{
#if __sysv
  return translate_code(code);
#endif

  // Unreachable
  assert(0);
  return -1;
}

}
