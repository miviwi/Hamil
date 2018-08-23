#pragma once

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/frame.h>
#include <ui/drawable.h>

#include <string>

namespace ui {

class LabelFrame : public Frame {
public:
  using Frame::Frame;
  virtual ~LabelFrame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  LabelFrame& caption(const std::string& caption);

  virtual vec2 sizeHint() const;

private:
  Drawable m_caption;
};

}