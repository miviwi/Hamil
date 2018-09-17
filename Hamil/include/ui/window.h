#pragma once

#include <ui/ui.h>
#include <ui/uicommon.h>
#include <ui/frame.h>
#include <ui/drawable.h>

namespace ui {

class WindowFrame : public Frame {
public:
  static constexpr vec2 DecorationsSize = { 0.0f, 18.0f };

  using Frame::Frame;
  virtual ~WindowFrame();
  
  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  Frame& position(vec2 pos);

  WindowFrame& title(const std::string& title);

  WindowFrame& content(Frame *content);
  WindowFrame& content(Frame& content);

  virtual vec2 sizeHint() const;

private:
  enum State {
    Default, Moving,
  };

  Geometry decorationsGeometry() const;

  State m_state = Default;

  Drawable m_title;
  Frame *m_content = nullptr;
};

}