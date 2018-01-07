#pragma once

#include "ui/common.h"
#include "ui/ui.h"
#include "ui/frame.h"
#include "font.h"

#include <functional>

namespace ui {

class VertexPainter;

class ButtonFrame : public Frame {
public:
  using OnClick = Signal<ButtonFrame *>;

  using Frame::Frame;
  virtual ~ButtonFrame();

  virtual bool input(ivec2 mouse_pos, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  ButtonFrame& caption(std::string caption);
  ButtonFrame& onClick(OnClick::Slot on_click);

  OnClick& click();

private:
  enum State {
    Default, Hover, Pressed
  };

  State m_state = Default;
  ft::String m_caption;
  OnClick m_on_click;
};


}