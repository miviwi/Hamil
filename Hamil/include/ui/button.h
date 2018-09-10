#pragma once

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/frame.h>
#include <ui/drawable.h>

#include <string>

namespace ui {

class VertexPainter;

class ButtonFrame : public Frame {
public:
  using Frame::Frame;
  virtual ~ButtonFrame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);

  void losingCapture();

protected:
  enum State {
    Default, Hover, Pressed,
  };

  void captionPaint(const Drawable& caption, VertexPainter& painter, Geometry parent, State state);

  vec2 captionSizeHint(const Drawable& caption) const;

  virtual Geometry getSolidGeometry() const = 0;
  virtual void emitClicked() = 0;

  State m_state = Default;
};

class PushButtonFrame : public ButtonFrame {
public:
  using OnClick = Signal<PushButtonFrame *>;

  using ButtonFrame::ButtonFrame;

  virtual void paint(VertexPainter& painter, Geometry parent);

  PushButtonFrame& caption(std::string caption);

  PushButtonFrame& onClick(OnClick::Slot on_click);
  OnClick& click();

  virtual vec2 sizeHint() const;

protected:
  virtual Geometry getSolidGeometry() const;
  virtual void emitClicked();

private:
  Drawable m_caption;
  OnClick m_on_click;
};

class ToggleButtonFrame : public ButtonFrame {
public:
  using OnClick = Signal<ToggleButtonFrame *>;

  using ButtonFrame::ButtonFrame;

  virtual void paint(VertexPainter& painter, Geometry parent);

  ToggleButtonFrame& caption(std::string caption);

  ToggleButtonFrame& value(bool value);
  bool value() const;

  ToggleButtonFrame& onClick(OnClick::Slot on_click);
  OnClick& click();

  virtual vec2 sizeHint() const;

protected:
  virtual Geometry getSolidGeometry() const;
  virtual void emitClicked();

private:
  bool m_value = true;
  Drawable m_caption;
  OnClick m_on_click;
};

class CheckBoxFrame : public ButtonFrame {
public:
  using OnClick = Signal<CheckBoxFrame *>;

  using ButtonFrame::ButtonFrame;

  virtual void paint(VertexPainter& painter, Geometry parent);

  CheckBoxFrame& value(bool value);
  bool value() const;

  CheckBoxFrame& onClick(OnClick::Slot on_click);
  OnClick& click();

  virtual vec2 sizeHint() const;

protected:
  virtual Geometry getSolidGeometry() const;
  virtual void emitClicked();

private:
  constexpr static float Dimensions = 17.0f;
  constexpr static float PixelMargin = 4.0f;

  bool m_value = true;
  OnClick m_on_click;
};


}