#include <os/input.h>

namespace os {

void Input::deleter(Input *ptr)
{
  if(auto mouse = ptr->get<Mouse>()) {
    delete mouse;

    return;
  } else if(auto kb = ptr->get<Keyboard>()) {
    delete kb;

    return;
  }

  assert(0);     // Unreachable
}

Input::Ptr Input::invalid_ptr()
{
  return Input::Ptr(nullptr, &Input::deleter);
}

Input::Ptr Mouse::create()
{
  return Input::Ptr(new Mouse(), &Input::deleter);
}

bool Mouse::buttonDown(Button btn) const
{
  return event == Down && ev_data == btn;
}

bool Mouse::buttonUp(Button btn) const
{
  return event == Up && ev_data == btn;
}

const char *Mouse::dbg_TypeStr() const
{
  switch(event) {
  case Move:  return "Move";
  case Up:    return "Up";
  case Down:  return "Down";
  case Wheel: return "Wheel";
  case DoubleClick: return "DoubleClick";
  }

  return "<Invalid>";
}

Input::Ptr Keyboard::create()
{
  return Input::Ptr(new Keyboard(), &Input::deleter);
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

const char *Keyboard::dbg_TypeStr() const
{
  switch(event) {
  case KeyUp:   return "KeyUp";
  case KeyDown: return "KeyDown";
  case SysUp:   return "SysUp";
  case SysDown: return "SysDown";
  }

  return "<Invalid>";
}

}
