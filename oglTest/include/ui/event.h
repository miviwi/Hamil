#pragma once

#include <common.h>
#include <ui/uicommon.h>
#include <ui/ui.h>

namespace ui {

class Event {
public:
  virtual ~Event() { }
};

class InputEvent : public Event {
public:
  const win32::Input *input() const;

protected:
  InputEvent(const InputPtr& input);

private:
  const win32::Input *m_input;
};

class MouseEvent : public InputEvent {
public:
  using Button = win32::Mouse::Button;
  using Event  = win32::Mouse::Event;

  const win32::Mouse *mouse() const;

  vec2 pos() const;
  ivec2 ipos() const;

  vec2 delta() const;

  unsigned buttons() const;

protected:
  MouseEvent(const InputPtr& input, CursorDriver& cursor);

  CursorDriver *m_cursor;
};

class MouseMoveEvent : public MouseEvent {
  friend class Ui;

protected:
  using MouseEvent::MouseEvent;
};

class MouseButtonEvent : public MouseEvent {
  friend class Ui;

public:
  Button button() const;
  Event event() const;

protected:
  using MouseEvent::MouseEvent;
};

class MouseDragEvent : public MouseEvent {
  friend class Ui;

public:
  enum MotionState {
    DragStart, Drag, DragEnd,
  };

  MotionState state() const;

  vec2 origin() const;
  vec2 dragDelta() const;

protected:
  MouseDragEvent(const InputPtr& input, CursorDriver& cursor,
    MotionState state, vec2 origin);

private:
  MotionState m_state;
  vec2 m_origin;

};

}