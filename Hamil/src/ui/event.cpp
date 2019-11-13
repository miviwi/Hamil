#include <ui/event.h>

namespace ui {

InputEvent::InputEvent(const InputPtr& input) :
  m_input(input.get())
{
}

const os::Input *InputEvent::input() const
{
  return m_input;
}

MouseEvent::MouseEvent(const InputPtr& input, CursorDriver& cursor) :
  InputEvent(input), m_cursor(&cursor)
{
}

const os::Mouse *MouseEvent::mouse() const
{
  // Don't need to use os::Input::get() as the input
  //   is guaranteed to be a os::Mouse
  return (const os::Mouse *)input();
}

vec2 MouseEvent::pos() const
{
  return m_cursor->pos();
}

ivec2 MouseEvent::ipos() const
{
  return m_cursor->ipos();
}

vec2 MouseEvent::delta() const
{
  return vec2{ mouse()->dx, mouse()->dy };
}

unsigned MouseEvent::buttons() const
{
  return mouse()->buttons;
}

MouseEvent::Button MouseButtonEvent::button() const
{
  return (Button)mouse()->ev_data;
}

MouseEvent::Event MouseButtonEvent::event() const
{
  return (Event)mouse()->event;
}

MouseDragEvent::MotionState MouseDragEvent::state() const
{
  return m_state;
}

vec2 MouseDragEvent::origin() const
{
  return m_origin;
}

vec2 MouseDragEvent::dragDelta() const
{
  return pos() - origin();
}

MouseDragEvent::MouseDragEvent(const InputPtr& input, CursorDriver& cursor,
  MotionState state, vec2 origin) :
  MouseEvent(input, cursor), m_state(state), m_origin(origin)
{


}

}
