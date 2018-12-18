#include <ui/scrollframe.h>

namespace ui {

ScrollFrame::~ScrollFrame()
{
  delete m_content;
}

bool ScrollFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  Geometry g = geometry();
  bool mouse_inside = g.intersect(cursor.pos());
  if(!mouse_inside) return false;

  using win32::Mouse;
  if(auto mouse = input->get<Mouse>()) {
    // Content is smaller than the ScrollFrame - so
    //   no srolling is possible
    if(!hasVScrollbar()) return true;

    if(mouse->event == Mouse::Wheel) {
      if(scrollMousewheel(mouse)) return true;
    } else if(mouse->buttons & Mouse::Left) {
      if(scrollbarClicked(cursor, mouse)) return true;
    } else if(mouse->buttonUp(Mouse::Left)) {
      m_ui->capture(nullptr);    // scrollBarClicked() calls m_ui->capture(this)
    }
  }

  return m_content ? m_content->input(cursor, input) : false;
}

void ScrollFrame::paint(VertexPainter& painter, Geometry parent)
{
  Geometry g = geometry();

  auto pipeline = gx::Pipeline()
    .premultAlphaBlend()
    .scissor(ui().scissorRect(parent.clip(g)))
    .primitiveRestart(Vertex::RestartIndex)
    ;

  if(m_content) {  // Paint the scrollbars
    painter
      .pipeline(pipeline)
      .rect(getVScrollbarGeometry(), grey().opacity(0.8))
      .rect(getHScrollbarGeometry(), grey().opacity(0.8))
      ;
  }

  if(m_content) m_content->paint(painter, parent.clip(g));
}

ScrollFrame& ScrollFrame::scrollbars(uint sb)
{
  m_scrollbars = sb;

  return *this;
}

ScrollFrame& ScrollFrame::content(Frame *frame)
{
  m_content = frame;

  m_content->attached(this);

  return *this;
}

ScrollFrame& ScrollFrame::content(Frame& frame)
{
  return content(&frame);
}

vec2 ScrollFrame::sizeHint() const
{
  return { 100.0f, 50.0f };
}

bool ScrollFrame::hasVScrollbar() const
{
  if(!m_content || !(m_scrollbars & VScrollbar)) return false;

  return m_content->size().y > size().y;
}

bool ScrollFrame::hasHScrollbar() const
{
  if(!m_content || !(m_scrollbars & HScrollbar)) return false;

  return m_content->size().x > size().x;
}

Geometry ScrollFrame::getVScrollbarGeometry() const
{
  if(!hasVScrollbar()) return Geometry::empty();

  Geometry g = geometry();
  vec2 content_sz = m_content->size(),
    visible_content = g.size() * content_sz.recip(),
    scrollbar_sz = vec2::max(visible_content, vec2(0.1f, 0.1f)) * g.size(),
    scrollbar_pos = -m_scroll * content_sz.recip(); // m_scroll < 0

  return {
    g.x + (g.w-ScrollbarSize), g.y + g.h*scrollbar_pos.y - ScrollbarMargin,
    ScrollbarSize, scrollbar_sz.y
  };
}

Geometry ScrollFrame::getHScrollbarGeometry() const
{
  if(!hasHScrollbar()) return Geometry::empty();

  Geometry g = geometry();
  vec2 content_sz = m_content->size(),
    visible_content = g.size() * content_sz.recip(),
    scrollbar_sz = vec2::max(visible_content, vec2(0.1f, 0.1f)) * g.size(),
    scrollbar_pos = -m_scroll * content_sz.recip(); // m_scroll < 0

  return {
    g.x + g.w*scrollbar_pos.x, g.y + (g.h-ScrollbarSize),
    scrollbar_sz.x, ScrollbarSize
  };
}

Geometry ScrollFrame::getVScrollbarRegion() const
{
  if(!hasVScrollbar()) return Geometry::empty();

  Geometry g = geometry();
  return {
    g.x + (g.w - ScrollbarSize) - ScrollbarClickMargin, g.y,
    ScrollbarSize + 2.0f*ScrollbarClickMargin, g.h - ScrollbarMargin
  };
}

Geometry ScrollFrame::getHScrollbarRegion() const
{
  if(!hasHScrollbar()) return Geometry::empty();

  Geometry g = geometry();
  return {
    g.x, g.y + (g.h - ScrollbarSize) - ScrollbarClickMargin,
    g.w - ScrollbarMargin, ScrollbarSize + 2.0f*ScrollbarClickMargin,
  };
}

void ScrollFrame::scrollContentY(float y)
{
  if(!m_content) return;

  Geometry g = geometry();
  vec2 content_sz = m_content ? m_content->size() : g.size();
  m_scroll.y = clamp(y, -(content_sz.y - g.size().y), 0.0f);

  m_content->position(m_scroll);
}

void ScrollFrame::scrollContentX(float x)
{
  if(!m_content) return;

  Geometry g = geometry();
  vec2 content_sz = m_content ? m_content->size() : g.size();
  m_scroll.x = clamp(x, -(content_sz.x - g.size().x), 0.0f);

  m_content->position(m_scroll);
}

bool ScrollFrame::scrollMousewheel(const win32::Mouse *mouse)
{
  float y = m_scroll.y + (5.0f*mouse->ev_data / 120.0f);
  scrollContentY(y);

  return true;
}

bool ScrollFrame::scrollbarClicked(CursorDriver& cursor, const win32::Mouse *mouse)
{
  Geometry g = geometry();
  auto cursor_pos = cursor.pos();

  // Relative click position
  auto relative_pos = cursor_pos - g.pos();

  if(auto v = getVScrollbarRegion(); v.intersect(cursor_pos)) {
    scrollContentY(-relative_pos.y);
  } else if(auto h = getHScrollbarRegion(); h.intersect(cursor_pos)) {
    scrollContentX(-relative_pos.x);
  } else {
    return false;
  }

  m_ui->capture(this);
  return true;
}

}