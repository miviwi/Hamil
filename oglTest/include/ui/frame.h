#pragma once

#include "ui/common.h"
#include "ui/ui.h"
#include "vmath.h"
#include "input.h"

namespace ui {

class Frame {
public:
  enum Gravity {
    Left, Center, Right,
  };

  Frame(Ui& ui, const char *name, Geometry geom);
  Frame(Ui& ui, Geometry geom);
  Frame(Ui& ui, const char *name);
  Frame(Ui& ui);
  virtual ~Frame();

  virtual bool input(ivec2 mouse_pos, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  Frame& geometry(Geometry geom);
  Geometry geometry() const;

  Frame& gravity(Gravity gravity);
  Gravity gravity() const;

protected:
  friend class Ui;

  bool mouseWillLeave(ivec2 mouse_pos, const win32::Mouse *mouse);

  Ui *m_ui;

  const char *m_name;
  Gravity m_gravity;
  Geometry m_geom;
  unsigned m_countrer = 0;
};

}