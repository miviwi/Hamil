#pragma once

#include <common.h>

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/cursor.h>
#include <ui/animation.h>
#include <ui/layout.h>
#include <ui/textbox.h>
#include <math/geometry.h>

#include <deque>
#include <string>

namespace ui {

class ConsoleBufferFrame;

// TODO:
//   - That animation is impossible to get right...
class ConsoleFrame : public Frame {
public:
  using OnCommand = Signal<ConsoleFrame *, const char *>;

  static constexpr vec2 ConsoleSize = { 1080, 320 };

  ConsoleFrame(Ui& ui, const char *name);
  ConsoleFrame(Ui& ui);
  virtual ~ConsoleFrame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  virtual void losingCapture();
  virtual void attached();

  ConsoleFrame& toggle();
  ConsoleFrame& dropped(bool val);
  ConsoleFrame& print(const char *str);
  ConsoleFrame& clear();

  ConsoleFrame& onCommand(OnCommand::Slot on_command);
  OnCommand& command();

  virtual vec2 sizeHint() const;

private:
  Animation m_dropdown = {
    make_animation_channel({
      keyframe(-ConsoleSize.y, 1.0f),
      keyframe(0.0f, 0.0f),
    }, EaseQuinticOut),
  };

  static constexpr Geometry make_geometry();

  TextBoxFrame *m_prompt;
  ConsoleBufferFrame *m_buffer;
  LayoutFrame *m_console;

  bool m_dropped = false;

  OnCommand m_on_command;
};

}
