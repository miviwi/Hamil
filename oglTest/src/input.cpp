#include "input.h"

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace win32 {

InputDeviceManager::InputDeviceManager()
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
    throw;
  }

  m_mouse_speed = 1.0f;
  m_mouse_buttons = 0;
}

void InputDeviceManager::setMouseSpeed(float speed)
{
  m_mouse_speed = speed;
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

    input = Input::Ptr(new Keyboard());
    auto kbi = (Keyboard *)input.get();

    switch(kb->Message) {
    case WM_KEYDOWN: kbi->event = Keyboard::Down; break;
    case WM_KEYUP: kbi->event = Keyboard::Up; break;
    case WM_SYSKEYDOWN: kbi->event = Keyboard::SysDown; break;
    case WM_SYSKEYUP: kbi->event = Keyboard::SysUp; break;

    default: break;
    }
    kbi->key = kb->VKey;

    break;
  }

  case RIM_TYPEMOUSE: {
    RAWMOUSE *m = &raw->data.mouse;

    input = Input::Ptr(new Mouse());
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

    default: break;
    }

    mi->dx = (float)m->lLastX*m_mouse_speed;
    mi->dy = (float)m->lLastY*m_mouse_speed;

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
  return event == Down && key == k;
}

bool Keyboard::keyUp(unsigned k) const
{
  return event == Up && key == k;
}

}
