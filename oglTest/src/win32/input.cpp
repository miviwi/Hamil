#include <win32/input.h>
#include <win32/panic.h>

#include <vector>
#include <cctype>

#include <Windows.h>

namespace win32 {

InputDeviceManager::InputDeviceManager() :
  m_mouse_buttons(0), m_kb_modifiers(0)
{
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
    panic("couldn't register input devices!", -6);
  }

  setMouseSpeed(1.0f);
  setDoubleClickSpeed(0.5f);

  m_capslock = (GetKeyState(VK_CAPITAL) & 1) ? Keyboard::CapsLock : 0; //  bit 0 == toggle state
}

void InputDeviceManager::setMouseSpeed(float speed)
{
  m_mouse_speed = speed;
}

void InputDeviceManager::setDoubleClickSpeed(float speed_seconds)
{
  m_dbl_click_speed = Timers::s_to_ticks(speed_seconds);
}

void InputDeviceManager::process(void *handle)
{
  UINT sz = 0;
  GetRawInputData((HRAWINPUT)handle, RID_INPUT, nullptr, &sz, sizeof(RAWINPUTHEADER));

  std::vector<byte> buffer(sz);
  GetRawInputData((HRAWINPUT)handle, RID_INPUT, buffer.data(), &sz, sizeof(RAWINPUTHEADER));

  auto raw = (RAWINPUT *)buffer.data();

  Input::Ptr input;

  switch(raw->header.dwType) {
  case RIM_TYPEKEYBOARD: {
    RAWKEYBOARD *kb = &raw->data.keyboard;

    if(kb->VKey == 0xFF) return; // no mapping - ignore

    input = Input::Ptr(new Keyboard());
    input->timestamp = win32::Timers::ticks();

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

    kbi->key = Keyboard::translate_key(kb->VKey, kbi->modifiers);
    kbi->sym = Keyboard::translate_sym(kb->VKey, kbi->modifiers);

    break;
  }

  case RIM_TYPEMOUSE: {
    RAWMOUSE *m = &raw->data.mouse;

    input = Input::Ptr(new Mouse());
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

    mi->dx = (float)m->lLastX*m_mouse_speed;
    mi->dy = (float)m->lLastY*m_mouse_speed;

    doDoubleClick(mi);

    if(mi->event == Mouse::Up)        m_mouse_buttons &= ~mi->ev_data;
    else if(mi->event == Mouse::Down) m_mouse_buttons |= mi->ev_data;
    mi->buttons = m_mouse_buttons;

    break;
  }

  default: break;
  }

  m_input_buffer.push_back(std::move(input));
}

Input::Ptr InputDeviceManager::getInput()
{
  Input::Ptr ptr;

  if(m_input_buffer.size()) {
    ptr = std::move(m_input_buffer.front());
    m_input_buffer.pop_front();
  }

  return ptr;
}

void InputDeviceManager::doDoubleClick(Mouse *mi)
{
  if(mi->event != Mouse::Down) {
    if(mi->dx > 5.0f || mi->dy > 5.0f) m_clicks.clear();

    return;
  }

  if(!m_clicks.empty()) {
    auto last_click = m_clicks.last();
    auto dt = mi->timestamp - last_click.timestamp;

    m_clicks.clear();

    if(last_click.ev_data == mi->ev_data && dt < m_dbl_click_speed) {
      mi->event = Mouse::DoubleClick;
      return;
    }
  }

  m_clicks.push(*mi);
}

bool Mouse::buttonDown(Button btn) const
{
  return event == Down && ev_data == btn;
}

bool Mouse::buttonUp(Button btn) const
{
  return event == Up && ev_data == btn;
}

bool Keyboard::keyDown(unsigned k) const
{
  return event == KeyDown && key == k;
}

bool Keyboard::keyUp(unsigned k) const
{
  return event == KeyUp && key == k;
}

bool Keyboard::modifier(unsigned mod) const
{
  return (modifiers & mod) == mod;
}

bool Keyboard::special() const
{
  return key > SpecialKey;
}

unsigned Keyboard::translate_sym(u16 vk, unsigned modifiers)
{
  bool shift = modifiers & Shift,
    caps = modifiers & CapsLock;

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

  return (shift || caps) ? toupper(vk) : tolower(vk);
}

unsigned Keyboard::translate_key(u16 vk, unsigned modifiers)
{
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

  case VK_ESCAPE: return Escape;
  case VK_F1:     return F1;
  case VK_F2:     return F2;
  case VK_F3:     return F3;
  case VK_F4:     return F4;
  case VK_F5:     return F5;
  case VK_F6:     return F6;
  case VK_F7:     return F7;
  case VK_F8:     return F8;
  case VK_F9:     return F9;
  case VK_F10:    return F10;
  case VK_F11:    return F11;
  case VK_F12:    return F12;

  case VK_UP:    return Up;
  case VK_DOWN:  return Down;
  case VK_LEFT:  return Left;
  case VK_RIGHT: return Right;

  case VK_RETURN: return Enter;
  case VK_BACK:   return Backspace;

  case VK_INSERT: return Insert;
  case VK_DELETE: return Delete;
  case VK_HOME:   return Home;
  case VK_END:    return End;
  case VK_PRIOR:  return PageUp;
  case VK_NEXT:   return PageDown;

  case VK_PRINT:  return Print;
  case VK_SCROLL: return ScrollLock;
  case VK_PAUSE:  return Pause;
  }

  return vk;
}

}
