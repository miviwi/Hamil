#pragma once

#include <common.h>
#include <math/geometry.h>
#include <win32/input.h>

namespace game {

using InputPtr = win32::Input::Ptr;

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
  void toggleVisible();

  vec2 pos() const;
  ivec2 ipos() const;

private:
  Type m_type;
  vec2 m_pos;
  bool m_shown;
};

}