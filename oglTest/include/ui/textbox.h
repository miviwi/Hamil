#pragma once

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/frame.h>

namespace ui {

class TextBoxFrame : public Frame {
public:
  using OnChange = Signal<TextBoxFrame *>;

  using Frame::Frame;
  virtual ~TextBoxFrame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

};

}