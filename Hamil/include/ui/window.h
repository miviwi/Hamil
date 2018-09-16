#pragma once

#include <ui/ui.h>
#include <ui/uicommon.h>
#include <ui/frame.h>

namespace ui {

class WindowFrame : public Frame {
public:
  static constexpr vec2 DecorationsSize = { 0.0f, 10.0f };

  using Frame::Frame;
  virtual ~WindowFrame();
  
  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  Frame& position(vec2 pos);

  WindowFrame& content(Frame *content);
  WindowFrame& content(Frame& content);

  virtual vec2 sizeHint() const;

private:
  Geometry decorationsGeometry() const;

  Frame *m_content = nullptr;
};

}