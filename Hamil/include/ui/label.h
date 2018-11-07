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
  LabelFrame& drawable(const Drawable& img);

  // Must be set BEFORE changing the caption or changes won't be seen
  LabelFrame& font(const ft::Font::Ptr& font);
  // Must be set BEFORE changing the caption or changes won't be seen
  LabelFrame& color(Color c);

  LabelFrame& background(Color bg);

  virtual vec2 sizeHint() const;

private:
  ft::Font::Ptr ownFont();

  ft::Font::Ptr m_font = nullptr;

  Drawable m_caption;

  Color m_color = white();
  Color m_bg    = transparent();
};

}