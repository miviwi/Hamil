#pragma once

#include <common.h>

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/cursor.h>
#include <ui/layout.h>
#include <ui/textbox.h>
#include <math/geometry.h>

#include <deque>
#include <string>

namespace ui {

class ConsoleBufferFrame;

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

  ConsoleFrame& onCommand(OnCommand::Slot on_command);
  OnCommand& command();

  virtual vec2 sizeHint() const;

private:
  static Geometry make_geometry();

  TextBoxFrame *m_prompt;
  ConsoleBufferFrame *m_buffer;
  LayoutFrame *m_console;

  OnCommand m_on_command;
};

class ConsoleBufferFrame : public Frame {
public:
  using LineBuffer = std::deque<std::string>;

  static constexpr float BufferHeight = 290.0f,
    BufferPixelMargin = 5.0f;

  enum {
    BufferDepth = 512,
  };

  using Frame::Frame;

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  virtual vec2 sizeHint() const;

  void submitCommand(TextBoxFrame *prompt);

  const std::string& historyPrevious();
  const std::string& historyNext();

private:

  LineBuffer m_buffer;
  int m_cursor = -1;
};

}
