#pragma once

#include <common.h>

#include <list>
#include <memory>

namespace win32 {

struct Input {
  using Ptr = std::unique_ptr<Input>;
  using Tag = const char *;

  template <typename T>
  T *get()
  {
    return getTag() == T::tag() ? (T *)this : nullptr;
  }

protected:
  virtual Tag getTag() const = 0;
};

struct Mouse : public Input {
  enum Button {
    Left   = 1<<0,
    Right  = 1<<1,
    Middle = 1<<2,
    X1     = 1<<3,
    X2     = 1<<4,
  };

  enum Event {
    Move, Up, Down, Wheel,
  };

  unsigned buttons;

  unsigned event;
  int ev_data;

  float dx, dy;

  static Tag tag() { return "mouse"; }

  bool buttonDown(Button btn) const;
  bool buttonUp(Button btn) const;

protected:
  virtual Tag getTag() const { return tag(); }
};

struct Keyboard : public Input {
  enum Event {
    Invalid,
    Up, Down,
    SysUp, SysDown,
  };

  unsigned event;
  unsigned key;

  static Tag tag() { return "kb"; }

  bool keyDown(unsigned k) const;
  bool keyUp(unsigned k) const;

protected:
  virtual Tag getTag() const { return tag(); }
};

struct InputDeviceManager {
public:
  InputDeviceManager();

  void setMouseSpeed(float speed);

  void process(void *handle);

  Input::Ptr getInput();

private:
  std::list<Input::Ptr> m_input_buffer;

  float m_mouse_speed;
  unsigned m_mouse_buttons;
};

}
