#pragma once

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <vmath.h>
#include <win32/input.h>

#include <utility>

namespace ui {

// TODO:
//   - Think of a way to eliminate the Ui& argument of constructor
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

  virtual vec2 sizeHint() const;

protected:
  friend class Ui;
  Ui *m_ui;

private:
  const char *m_name;
  Gravity m_gravity;
  Geometry m_geom;
};

template <typename T, typename... Args>
static T& create(Args&&... args)
{
  return *(new T(std::forward<Args>(args)...));
}

}