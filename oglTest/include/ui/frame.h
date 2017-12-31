#pragma once

#include "ui/common.h"
#include "ui/ui.h"
#include "vmath.h"
#include "input.h"

namespace ui {

class Frame {
public:
  enum PositionMode {
    Local, Global,
  };

  Frame(Ui *ui, Geometry geom);

  virtual bool input(ivec2 mouse_pos, const InputPtr& input);

protected:
  friend class Ui;

  virtual void paint(VertexPainter& painter, Geometry parent);

  Ui *m_ui;

  PositionMode m_pos_mode;
  Geometry m_geom;
  unsigned m_countrer = 0;
};

}