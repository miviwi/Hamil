#include <ui/layout.h>

#include <algorithm>

namespace ui {

LayoutFrame::~LayoutFrame()
{
  for(const auto& frame : m_frames) delete frame;
}

bool LayoutFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  bool mouse_inside = geometry().intersect(cursor.pos());
  if(!mouse_inside) return false;

  for(auto iter = m_frames.crbegin(); iter != m_frames.crend(); iter++) {
    const auto& frame = *iter;
    if(frame->input(cursor, input)) return true;
  }
  return false;
}

void LayoutFrame::paint(VertexPainter& painter, Geometry parent)
{
  Geometry g = parent.clip(geometry());
  reflow();

  auto pipeline = gx::Pipeline()
    .premultAlphaBlend()
    .primitiveRestart(Vertex::RestartIndex)
    .noScissor()
    ;

  if(m_dbg_bboxes) {
    painter
      .pipeline(pipeline)
      .border(g.expand(1), 2.0f, red())
      ;
  }

  for(const auto& frame : m_frames) {
    if(m_dbg_bboxes && !frame->isLayout()) {
      painter
        .pipeline(pipeline)
        .border(frame->geometry(), 1.0f, green())
        ;
    }

    frame->paint(painter, g);
  }
}

LayoutFrame& LayoutFrame::dbg_DrawBBoxes(bool enabled)
{
  m_dbg_bboxes = enabled;
  return *this;
}

LayoutFrame& LayoutFrame::frame(Frame *frame)
{
  frame->attached(this);
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
    vec2 fsz = frame->size();

    size.x = std::max(fsz.x, size.x);
    size.y += fsz.y;
  }

  return size;
}

void RowLayoutFrame::reflow()
{
  Geometry g = geometry();
  vec2 center = g.centerRelative();

  float y = 0.0f;

  for(const auto& frame : m_frames) {
    vec2 fsz = frame->size();
    Geometry calculated = {
      0.0f, y,
      fsz.x ? fsz.x : g.w, fsz.y
    };

    switch(frame->gravity()) {
    case Frame::Center: calculated.x = center.x - (fsz.x/2.0f); break;
    case Frame::Right:  calculated.x = g.w-fsz.x; break;
    }

    frame->geometry(calculated);

    y += calculated.h;
  }
}

ColumnLayoutFrame::~ColumnLayoutFrame()
{
}

vec2 ColumnLayoutFrame::sizeHint() const
{
  vec2 size = { 0, -1 };
  for(const auto& frame : m_frames) {
    vec2 fsz = frame->size();

    size.x += fsz.x;
    size.y = std::max(fsz.y, size.y);
  }

  return size;
}

void ColumnLayoutFrame::reflow()
{
  Geometry g = geometry();
  vec2 center = g.centerRelative();

  float x = 0.0f;

  for(const auto& frame : m_frames) {
    vec2 fsz = frame->size();
    Geometry calculated = {
      x, 0.0f,
      fsz.x, fsz.y ? fsz.y : g.h
    };

    switch(frame->gravity()) {
    case Frame::Center: calculated.y = center.y - (fsz.y/2.0f); break;
    case Frame::Bottom: calculated.y = g.h-fsz.y; break;
    }

    frame->geometry(calculated);

    x += calculated.w;
  }
}

}