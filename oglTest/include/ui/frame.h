#pragma once

#include "ui/common.h"
#include "ui/ui.h"
#include "vmath.h"
#include "input.h"

#include <utility>

namespace ui {

class Frame {
public:
  enum Gravity {
    Left, Right,
    Top, Bottom,
    Center,
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

  virtual void losingCapture();

protected:
  friend class Ui;

  Ui *m_ui;

  const char *m_name;
  Gravity m_gravity;
  Geometry m_geom;
  unsigned m_countrer = 0;
};

template <typename T, typename... Args>
static T& create(Args&&... args)
{
  return *(new T(std::forward<Args>(args)...));
}

}