#pragma once

#include <common.h>

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/cursor.h>
#include <ui/event.h>
#include <ui/animation.h>
#include <math/geometry.h>
#include <win32/input.h>

#include <utility>

namespace ui {

// TODO:
//   - Think of a way to eliminate the Ui& argument of constructor
// 
//   - Refactor input handling to make use of events.
//     That is instead of input() being unique to each widget
//     'Frame' will emit calls to ex. :
//       * evMouseEnter()
//       * evMouseClick()
//       * evMouseDrag()
//       * etc...
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

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  Frame& geometry(Geometry geom);
  Geometry geometry() const;
  Frame& gravity(Gravity gravity);
  Gravity gravity() const;

  void position(vec2 pos);

  virtual void losingCapture();
  virtual void attached();

  virtual vec2 sizeHint() const;

  virtual bool isLayout() const { return false; }

protected:
  friend class Ui;
  Ui *m_ui;

  // All the ev* functions return a bool which stops event
  //   bubbling when true

  virtual bool evMouseEnter(const MouseMoveEvent& e);
  virtual bool evMouseLeave(const MouseMoveEvent& e);

  virtual bool evMouseMove(const MouseMoveEvent& e);

  virtual bool evMouseDown(const MouseButtonEvent& e);
  virtual bool evMouseUp(const MouseButtonEvent& e);

  virtual bool evMouseDrag(const MouseDragEvent& e);
  
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