#pragma once

#include <common.h>

#include "vmath.h"
#include "input.h"
#include "window.h"
#include "ui/common.h"
#include "ui/style.h"

#include <vector>

namespace ui {

void init();
void finalize();

class VertexPainter;
class Frame;

using InputPtr = win32::Input::Ptr;

class Ui {
public:
  static const vec2 FramebufferSize;

  Ui(Geometry geom, const Style& style);
  ~Ui();

  static ivec4 scissor_rect(Geometry g);

  void frame(Frame *frame);
  const Style& style() const;

  bool input(ivec2 mouse_pos, const InputPtr& input);
  void paint();

private:
  Geometry m_geom;
  Style m_style;
  Frame *m_frame;
};

}