#pragma once

#include <common.h>

#include <util/ringbuffer.h>
#include <win32/time.h>

#include <list>
#include <memory>

namespace win32 {

struct Input {
  using Ptr = std::unique_ptr<Input>;
  using Tag = const char *;

  // Use example:
  //   if(auto mouse = input.get<Mouse>()) { ... }
  template <typename T>
  T *get()
  {
    return getTag() == T::tag() ? (T *)this : nullptr;
  }

  Time timestamp;

protected:
  virtual Tag getTag() const = 0;
};

struct Mouse : public Input {
  enum Button : u16 {
    Left   = 1<<0,
    Right  = 1<<1,
    Middle = 1<<2,
    X1     = 1<<3,
    X2     = 1<<4,

    DoubleClick = 1<<15,
  };

  enum Event : u16 {
    Move, Up, Down, Wheel,
  };

  u16 event;

  u16 buttons;
  int ev_data;

  float dx, dy;

  static Tag tag() { return "mouse"; }

  bool buttonDown(Button btn) const;
  bool buttonUp(Button btn) const;

protected:
  virtual Tag getTag() const { return tag(); }
};

struct Keyboard : public Input {
  enum Event : u16 {
    Invalid,
    KeyUp, KeyDown,
    SysUp, SysDown,
  };

  enum Modifier : u16 {
    Ctrl  = 1<<0,
    Shift = 1<<1,
    Alt   = 1<<2,
    Super = 1<<3,

    CapsLock = 1<<15,
  };

  enum Key {
    SpecialKey = (1<<16),

    Up, Left, Down, Right,
    
    Enter, Backspace,

    Insert, Home, PageUp,
    Delete, End,  PageDown,

    Print, ScrollLock, Pause,
  };

  u16 event;
  u16 modifiers;

  unsigned key; // raw scancode
  unsigned sym; // printed character

  static Tag tag() { return "kb"; }

  bool keyDown(unsigned k) const;
  bool keyUp(unsigned k) const;

  bool modifier(unsigned mod) const;

  bool special() const;

protected:
  virtual Tag getTag() const { return tag(); }

private:
  friend class InputDeviceManager;

  static unsigned translate_key(u16 vk, unsigned modifiers);
  static unsigned translate_sym(u16 vk, unsigned modifiers);
};

class InputDeviceManager {
public:
  InputDeviceManager();

  void setMouseSpeed(float speed);
  void setDoubleClickSpeed(float speed_seconds);

  void process(void *handle);

  Input::Ptr getInput();

private:
  void doDoubleClick(Mouse *mi);

  std::list<Input::Ptr> m_input_buffer;

  float m_mouse_speed;
  unsigned m_mouse_buttons;

  unsigned m_kb_modifiers;
  unsigned m_capslock;

  RingBuffer<Mouse> m_clicks;
  Time m_dbl_click_speed;
};

}
