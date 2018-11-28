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

  // Sets the Frame's Geometry and it's name - which
  //   can be used to get it via Ui::getFrameByName()
  // The storage for 'name' can be discarded after the
  //   constructor returns ('ui' keeps an internal copy)
  // The Frame is NOT made a child of the Ui explicitly
  //   if that is desired call Ui::frame()
  Frame(Ui& ui, const char *name, Geometry geom);

  // Sets the Frame's geometry
  Frame(Ui& ui, Geometry geom);

  // The storage for 'name' can be discarded after the
  //   constructor returns. See note for
  //       Frame(Ui&, const char *name, Geometry)
  Frame(Ui& ui, const char *name);
  Frame(Ui& ui);
  virtual ~Frame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  Frame& geometry(Geometry geom);
  Geometry geometry() const;
  Frame& gravity(Gravity gravity);
  Gravity gravity() const;

  // Set the Frame's position relative to it's parent
  Frame& position(vec2 pos);
  // Returns the Frame's position relative to the Ui
  vec2 position() const;
  // Returns the Frame's position relative to it's parent
  vec2 positionRelative() const;

  Frame& size(vec2 sz);
  // Returns the size given to the Frame or the sizeHint()
  //   which will be >= padding()
  vec2 size() const;

  vec2 padding() const;
  // Sets the padding of a Frame to 'pad' which means
  //   the Frames size will be at least as big as 'pad'
  Frame& padding(vec2 pad);

  // Intended for internal use (called when the focus
  //   is taken by a different Frame)
  virtual void losingCapture();
  // Intended for internal use (called when the Frame
  //   is attached to the Ui, a Layout etc.)
  // When overriding this method Frame::attached(...)
  //   MUST be called inside it!
  virtual void attached(Frame *parent);

  // Used as the size when it's not explicitly specified
  //   - The actual value depends on the Frame's type,
  //     for, say a LabelFrame it'll be the size needed
  //     to contain the caption
  virtual vec2 sizeHint() const;

  // Intended for internal use
  virtual bool isLayout() const { return false; }

protected:
  friend class Ui;
  Ui *m_ui;

  Ui& ui();
  const Ui& ui() const;

  // TODO
  const Style& ownStyle() const;

  // All the ev* functions return a bool which stops event
  //   bubbling when true

  virtual bool evMouseEnter(const MouseMoveEvent& e);
  virtual bool evMouseLeave(const MouseMoveEvent& e);

  virtual bool evMouseMove(const MouseMoveEvent& e);

  virtual bool evMouseDown(const MouseButtonEvent& e);
  virtual bool evMouseUp(const MouseButtonEvent& e);

  virtual bool evMouseDrag(const MouseDragEvent& e);
  
private:
  Frame *m_parent;

  const char *m_name;
  Gravity m_gravity;
  Geometry m_geom;
  vec2 m_pad;
};

// Returns a reference to a HEAP allocated Frame
//   - A reference is returned instead of a pointer
//     so method chaining looks nicer :)
//   - And for this reason all methods which attach
//     Frames to a parent (ex. Ui::frame()) have
//     overloads which take them by reference
template <typename T, typename... Args>
static T& create(Args&&... args)
{
  return *(new T(std::forward<Args>(args)...));
}

}