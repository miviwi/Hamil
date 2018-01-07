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
  using OnClickFn = std::function<void(ButtonFrame *)>;

  using Frame::Frame;
  virtual ~ButtonFrame();

  virtual bool input(ivec2 mouse_pos, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  ButtonFrame& caption(std::string caption);
  ButtonFrame& onClick(OnClickFn on_click);

private:
  enum State {
    Default, Hover, Pressed
  };

  State m_state = Default;
  ft::String m_caption;
  OnClickFn m_on_click;
};


}