#pragma once

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/frame.h>

#include <string>

namespace ui {

class LabelFrame : public Frame {
public:
  using Frame::Frame;
  virtual ~LabelFrame();

  virtual bool input(ivec2 mouse_pos, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  LabelFrame& caption(const std::string& caption);
  const std::string& caption() const;

  virtual vec2 sizeHint() const;

private:
  std::string m_caption;
};

}