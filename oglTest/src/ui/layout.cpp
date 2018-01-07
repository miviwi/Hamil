#include "ui/layout.h"

namespace ui {

StackLayoutFrame::~StackLayoutFrame()
{
  for(const auto& frame : m_frames) delete frame;
}

bool StackLayoutFrame::input(ivec2 mouse_pos, const InputPtr& input)
{
  if(!m_geom.intersect(mouse_pos)) return false;

  //Frame::input(mouse_pos, input);

  for(auto iter = m_frames.crbegin(); iter != m_frames.crend(); iter++) {
    const auto& frame = *iter;
    if(frame->input(mouse_pos, input)) return true;
  }
  return false;
}

StackLayoutFrame& StackLayoutFrame::frame(Frame *frame)
{
  m_frames.push_back(frame);
  calculateFrameGeometries();

  return *this;
}

void StackLayoutFrame::paint(VertexPainter& painter, Geometry parent)
{
  Geometry g = parent.clip(m_geom);
  calculateFrameGeometries();

  for(const auto& frame : m_frames) frame->paint(painter, g);
}

Frame& StackLayoutFrame::getFrameByIndex(unsigned idx)
{
  return *m_frames[idx];
}

void StackLayoutFrame::calculateFrameGeometries()
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

}