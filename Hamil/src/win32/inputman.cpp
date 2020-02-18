#include "os/time.h"
#include <win32/inputman.h>
#include <win32/panic.h>
#include <win32/time.h>

#include <config>

#include <algorithm>
#include <array>
#include <vector>

#include <cctype>
#include <cassert>

#if __win32
#  include <Windows.h>
#endif

namespace win32 {

struct KeyboardKeyState {
  bool down = false;
  os::Time pressed_timestamp = os::InvalidTime;
};

struct InputDeviceManagerData {
  std::array<KeyboardKeyState, 256> kb_keys;
};

static unsigned translate_key(u16 vk, unsigned modifiers);
static unsigned translate_sym(u16 vk, unsigned modifiers);

InputDeviceManager::InputDeviceManager() :
  m_data(nullptr)
{
#if __win32
  m_data = new InputDeviceManagerData();

  RAWINPUTDEVICE rid[2];

  // Mouse
  rid[0].usUsagePage = 0x01;
  rid[0].usUsage = 0x02;
  rid[0].dwFlags = 0;
  rid[0].hwndTarget = nullptr;

  // Keyboard
  rid[1].usUsagePage = 0x01;
  rid[1].usUsage = 0x06;
  rid[1].dwFlags = 0;
  rid[1].hwndTarget = nullptr;

  if(RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE)) == FALSE) {
    panic("couldn't register input devices!", InputDeviceRegistartionError);
  }

  m_kb_modifiers = 0;
  m_capslock = (GetKeyState(VK_CAPITAL) & 1) ? Keyboard::CapsLock : 0; //  bit 0 == toggle state
#endif
}

InputDeviceManager::~InputDeviceManager()
{
  delete m_data;
}

Input::Ptr InputDeviceManager::doPollInput()
{
#if __win32
  UINT sz = 0;
  GetRawInputData((HRAWINPUT)handle, RID_INPUT, nullptr, &sz, sizeof(RAWINPUTHEADER));

  std::vector<byte> buffer(sz);
  GetRawInputData((HRAWINPUT)handle, RID_INPUT, buffer.data(), &sz, sizeof(RAWINPUTHEADER));

  auto raw = (RAWINPUT *)buffer.data();

  Input::Ptr input(nullptr, &Input::deleter);

  switch(raw->header.dwType) {
  case RIM_TYPEKEYBOARD: {
    RAWKEYBOARD *kb = &raw->data.keyboard;

    if(kb->VKey == 0xFF) return; // no mapping - ignore

    auto timestamp = win32::Timers::ticks();

    input = Input::Ptr(new Keyboard(), &Input::deleter);
    input->timestamp = timestamp;

    auto kbi = (Keyboard *)input.get();

    switch(kb->Message) {
    case WM_KEYDOWN:    kbi->event = Keyboard::KeyDown; break;
    case WM_KEYUP:      kbi->event = Keyboard::KeyUp; break;
    case WM_SYSKEYDOWN: kbi->event = Keyboard::SysDown; break;
    case WM_SYSKEYUP:   kbi->event = Keyboard::SysUp; break;

    default: break;
    }
    
    unsigned modifiers = 0;
    switch(kb->VKey) {
    case VK_CONTROL: modifiers = Keyboard::Ctrl; break;
    case VK_SHIFT:   modifiers = Keyboard::Shift; break;
    case VK_MENU:    modifiers = Keyboard::Alt; break;
    case VK_LWIN:    modifiers = Keyboard::Super; break;
    case VK_RWIN:    modifiers = Keyboard::Super; break;
    }

    if(kbi->event == Keyboard::KeyUp) {
      m_kb_modifiers &= ~modifiers;
    } else if(kbi->event == Keyboard::KeyDown) {
      m_kb_modifiers |= modifiers;
      if(kb->VKey == VK_CAPITAL) m_capslock ^= Keyboard::CapsLock; // Toggles between Capslock and 0
    }
    kbi->modifiers = m_kb_modifiers | m_capslock;

    // Populate Keyboard::time_held
    //   - Update m_data->kb_keys as well
    auto& key_data = m_data->kb_keys.at(kb->VKey);
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

    kbi->key = translate_key(kb->VKey, kbi->modifiers);
    kbi->sym = translate_sym(kb->VKey, kbi->modifiers);

    break;
  }

  case RIM_TYPEMOUSE: {
    RAWMOUSE *m = &raw->data.mouse;

    input = Input::Ptr(new Mouse(), &Input::deleter);
    input->timestamp = win32::Timers::ticks();

    auto mi = (Mouse *)input.get();
    
    switch(m->usButtonFlags) {
    case RI_MOUSE_LEFT_BUTTON_DOWN:
      mi->event = Mouse::Down;
      mi->ev_data = Mouse::Left;
      break;
    case RI_MOUSE_RIGHT_BUTTON_DOWN:
      mi->event = Mouse::Down;
      mi->ev_data = Mouse::Right;
      break;
    case RI_MOUSE_MIDDLE_BUTTON_DOWN:
      mi->event = Mouse::Down;
      mi->ev_data = Mouse::Middle;
      break;
    case RI_MOUSE_BUTTON_4_DOWN:
      mi->event = Mouse::Down;
      mi->ev_data = Mouse::X1;
      break;
    case RI_MOUSE_BUTTON_5_DOWN:
      mi->event = Mouse::Down;
      mi->ev_data = Mouse::X2;
      break;

    case RI_MOUSE_LEFT_BUTTON_UP:
      mi->event = Mouse::Up;
      mi->ev_data = Mouse::Left;
      break;
    case RI_MOUSE_RIGHT_BUTTON_UP:
      mi->event = Mouse::Up;
      mi->ev_data = Mouse::Right;
      break;
    case RI_MOUSE_MIDDLE_BUTTON_UP:
      mi->event = Mouse::Up;
      mi->ev_data = Mouse::Middle;
      break;
    case RI_MOUSE_BUTTON_4_UP:
      mi->event = Mouse::Up;
      mi->ev_data = Mouse::X1;
      break;
    case RI_MOUSE_BUTTON_5_UP:
      mi->event = Mouse::Up;
      mi->ev_data = Mouse::X2;
      break;

    case RI_MOUSE_WHEEL:
      mi->event = Mouse::Wheel;
      mi->ev_data = (short)m->usButtonData;
      break;

    default: mi->event = Mouse::Move; break;
    }

    mi->dx = (float)m->lLastX*mouseSpeed();
    mi->dy = (float)m->lLastY*mouseSpeed();

    doDoubleClick(mi);

    if(mi->event == Mouse::Up)        m_mouse_buttons &= ~mi->ev_data;
    else if(mi->event == Mouse::Down) m_mouse_buttons |= mi->ev_data;
    mi->buttons = m_mouse_buttons;

    break;
  }

  default: break;
  }

  return input;
#else
  return Input::Ptr(nullptr, &Input::deleter);
#endif
}

static unsigned translate_sym(u16 vk, unsigned modifiers)
{
  bool shift = modifiers & Keyboard::Shift,
       caps = modifiers & Keyboard::CapsLock;

  bool upper = false;
  if(caps) {
    upper = !shift; // If CapsLock is currently ON Shift's function is inversed for letters
  } else {
    upper = shift;
  }

#if __win32
  if(shift) {
    switch(vk) {
    case '1': return '!';
    case '2': return '@';
    case '3': return '#';
    case '4': return '$';
    case '5': return '%';
    case '6': return '^';
    case '7': return '&';
    case '8': return '*';
    case '9': return '(';
    case '0': return ')';

    case VK_OEM_1:      return ':';
    case VK_OEM_2:      return '?';
    case VK_OEM_3:      return '~';
    case VK_OEM_4:      return '{';
    case VK_OEM_5:      return '|';
    case VK_OEM_6:      return '}';
    case VK_OEM_7:      return '\"';
    case VK_OEM_COMMA:  return '<';
    case VK_OEM_PERIOD: return '>';
    case VK_OEM_PLUS:   return '+';
    case VK_OEM_MINUS:  return '_';
    }
  } else {
    switch(vk) {
    case VK_OEM_1:      return ';';
    case VK_OEM_2:      return '/';
    case VK_OEM_3:      return '`';
    case VK_OEM_4:      return '[';
    case VK_OEM_5:      return '\\';
    case VK_OEM_6:      return ']';
    case VK_OEM_7:      return '\'';
    case VK_OEM_COMMA:  return ',';
    case VK_OEM_PERIOD: return '.';
    case VK_OEM_PLUS:   return '=';
    case VK_OEM_MINUS:  return '-';
    }
  }

  // Shift, Alt, CapsLock etc. do not have corresponding Keyboard::sym values
  switch(vk) {
  case VK_CONTROL:
  case VK_SHIFT:
  case VK_MENU:
  case VK_LWIN:
  case VK_RWIN: return 0;
  }

  return upper ? toupper(vk) : tolower(vk);
#else
  assert(0);   // Unreachable
#endif
}

unsigned translate_key(u16 vk, unsigned modifiers)
{
#if __win32
  switch(vk) {
  case VK_OEM_1:      return ';';
  case VK_OEM_2:      return '/';
  case VK_OEM_3:      return '`';
  case VK_OEM_4:      return '[';
  case VK_OEM_5:      return '\\';
  case VK_OEM_6:      return ']';
  case VK_OEM_7:      return '\'';
  case VK_OEM_COMMA:  return ',';
  case VK_OEM_PERIOD: return '.';
  case VK_OEM_PLUS:   return '+';
  case VK_OEM_MINUS:  return '-';

  case VK_ESCAPE: return Keyboard::Escape;
  case VK_F1:     return Keyboard::F1;
  case VK_F2:     return Keyboard::F2;
  case VK_F3:     return Keyboard::F3;
  case VK_F4:     return Keyboard::F4;
  case VK_F5:     return Keyboard::F5;
  case VK_F6:     return Keyboard::F6;
  case VK_F7:     return Keyboard::F7;
  case VK_F8:     return Keyboard::F8;
  case VK_F9:     return Keyboard::F9;
  case VK_F10:    return Keyboard::F10;
  case VK_F11:    return Keyboard::F11;
  case VK_F12:    return Keyboard::F12;

  case VK_UP:    return Keyboard::Up;
  case VK_DOWN:  return Keyboard::Down;
  case VK_LEFT:  return Keyboard::Left;
  case VK_RIGHT: return Keyboard::Right;

  case VK_TAB:    return Keyboard::Tab;
  case VK_RETURN: return Keyboard::Enter;
  case VK_BACK:   return Keyboard::Backspace;

  case VK_INSERT: return Keyboard::Insert;
  case VK_DELETE: return Keyboard::Delete;
  case VK_HOME:   return Keyboard::Home;
  case VK_END:    return Keyboard::End;
  case VK_PRIOR:  return Keyboard::PageUp;
  case VK_NEXT:   return Keyboard::PageDown;

  case VK_NUMLOCK:  return Keyboard::NumLock;
  case VK_SNAPSHOT: return Keyboard::Print;
  case VK_SCROLL:   return Keyboard::ScrollLock;
  case VK_PAUSE:    return Keyboard::Pause;

  case VK_CONTROL: return Keyboard::SpecialKey | Keyboard::Ctrl;
  case VK_SHIFT:   return Keyboard::SpecialKey | Keyboard::Shift;
  case VK_MENU:    return Keyboard::SpecialKey | Keyboard::Alt;
  case VK_LWIN:    // Fall-thru
  case VK_RWIN:    return Keyboard::SpecialKey | Keyboard::Super;
  case VK_CAPITAL: return Keyboard::SpecialKey | Keyboard::CapsLock;
  }
#else
  assert(0);   // Unreachable
#endif

  return vk;
}

}
