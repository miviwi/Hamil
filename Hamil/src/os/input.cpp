#include <os/input.h>

namespace os {

void Input::deleter(Input *ptr)
{
  if(auto mouse = ptr->get<Mouse>()) {
    delete mouse;
  } else if(auto kb = ptr->get<Keyboard>()) {
    delete kb;
  }

  assert(0);     // Unreachable
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

}
