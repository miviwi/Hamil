#include "ui/layout.h"

namespace ui {

LayoutFrame::~LayoutFrame()
{
  for(const auto& frame : m_frames) delete frame;
}

bool LayoutFrame::input(ivec2 mouse_pos, const InputPtr& input)
{
  if(!m_geom.intersect(mouse_pos)) return false;

  for(auto iter = m_frames.crbegin(); iter != m_frames.crend(); iter++) {
    const auto& frame = *iter;
    if(frame->input(mouse_pos, input)) return true;
  }
  return false;
}

void LayoutFrame::paint(VertexPainter& painter, Geometry parent)
{
  Geometry g = parent.clip(m_geom);
  calculateFrameGeometries();

  for(const auto& frame : m_frames) frame->paint(painter, g);
}

LayoutFrame& LayoutFrame::frame(Frame *frame)
{
  m_frames.push_back(frame);
  calculateFrameGeometries();

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

void RowLayoutFrame::calculateFrameGeometries()
{
  vec2 center = m_geom.center();

  float y = m_geom.y;

  for(const auto& frame : m_frames) {
    Geometry fg = frame->geometry();
    Geometry g = {
      0, y,
      fg.w ? fg.w : m_geom.w, fg.h
    };

    switch(frame->gravity()) {
    case Frame::Left:   g.x = m_geom.x; break;
    case Frame::Center: g.x = center.x - (fg.w/2.0f); break;
    case Frame::Right:  g.x = m_geom.x + (m_geom.w-fg.w); break;
    }

    frame->geometry(g);

    y += fg.h;
  }
}

ColumnLayoutFrame::~ColumnLayoutFrame()
{
}

void ColumnLayoutFrame::calculateFrameGeometries()
{
  vec2 center = m_geom.center();

  float x = m_geom.x;

  for(const auto& frame : m_frames) {
    Geometry fg = frame->geometry();
    Geometry g = {
      x, 0,
      fg.w, fg.h ? fg.h : m_geom.h
    };

    switch(frame->gravity()) {
    case Frame::Top:    g.y = m_geom.y; break;
    case Frame::Center: g.y = center.y - (fg.h/2.0f); break;
    case Frame::Bottom: g.y = m_geom.y + (m_geom.h-fg.h); break;
    }

    frame->geometry(g);

    x += fg.w;
  }
}

}