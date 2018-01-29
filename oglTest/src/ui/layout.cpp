#include <ui/layout.h>

#include <algorithm>

namespace ui {

LayoutFrame::~LayoutFrame()
{
  for(const auto& frame : m_frames) delete frame;
}

bool LayoutFrame::input(ivec2 mouse_pos, const InputPtr& input)
{
  if(!geometry().intersect(mouse_pos)) return false;

  for(auto iter = m_frames.crbegin(); iter != m_frames.crend(); iter++) {
    const auto& frame = *iter;
    if(frame->input(mouse_pos, input)) return true;
  }
  return false;
}

void LayoutFrame::paint(VertexPainter& painter, Geometry parent)
{
  Geometry g = parent.clip(geometry());
  calculateFrameGeometries();

  for(const auto& frame : m_frames) frame->paint(painter, g);
}

LayoutFrame& LayoutFrame::frame(Frame *frame)
{
  m_frames.push_back(frame);

  return *this;
}

LayoutFrame& LayoutFrame::frame(Frame& f)
{
  return frame(&f);
}

Frame& LayoutFrame::getFrameByIndex(unsigned idx)
{
  return *m_frames[idx];
}

RowLayoutFrame::~RowLayoutFrame()
{
}

vec2 RowLayoutFrame::sizeHint() const
{
  vec2 size = { -1, 0 };
  for(const auto& frame : m_frames) {
    Geometry fg = frame->geometry();

    size.x = std::max(fg.w, size.x);
    size.y += fg.h;
  }

  return size;
}

void RowLayoutFrame::calculateFrameGeometries()
{
  Geometry g = geometry();
  vec2 center = g.center();

  float y = g.y;

  for(const auto& frame : m_frames) {
    Geometry fg = frame->geometry();
    Geometry calculated = {
      g.x, y,
      fg.w ? fg.w : g.w, fg.h
    };

    switch(frame->gravity()) {
    case Frame::Center: calculated.x = center.x - (fg.w/2.0f); break;
    case Frame::Right:  calculated.x = g.x + (g.w-fg.w); break;
    }

    frame->geometry(calculated);

    y += fg.h;
  }
}

ColumnLayoutFrame::~ColumnLayoutFrame()
{
}

vec2 ColumnLayoutFrame::sizeHint() const
{
  vec2 size = { 0, -1 };
  for(const auto& frame : m_frames) {
    Geometry fg = frame->geometry();

    size.x += fg.w;
    size.y = std::max(fg.h, size.y);
  }

  return size;
}

void ColumnLayoutFrame::calculateFrameGeometries()
{
  Geometry g = geometry();
  vec2 center = g.center();

  float x = g.x;

  for(const auto& frame : m_frames) {
    Geometry fg = frame->geometry();
    Geometry calculated = {
      x, g.y,
      fg.w, fg.h ? fg.h : g.h
    };

    switch(frame->gravity()) {
    case Frame::Center: calculated.y = center.y - (fg.h/2.0f); break;
    case Frame::Bottom: calculated.y = g.y + (g.h-fg.h); break;
    }

    frame->geometry(calculated);

    x += fg.w;
  }
}

}