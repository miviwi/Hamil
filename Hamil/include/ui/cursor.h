#pragma once

#include <common.h>
#include <math/geometry.h>
#include <os/input.h>

namespace ui {

using InputPtr = os::Input::Ptr;

class CursorDriver {
public:
  enum Type {
    Default,

    NumTypes
  };

  static void init();
  static void finalize();

  CursorDriver(float x, float y);

  void input(const InputPtr& input);
  void paint();

  void type(Type type);

  void visible(bool visible);
  bool visible() const;
  void toggleVisible();

  vec2 pos() const;
  ivec2 ipos() const;

private:
  Type m_type;
  vec2 m_pos;
  bool m_shown;
};

}
