#include <ui/label.h>

namespace ui {

LabelFrame::~LabelFrame()
{
}

bool LabelFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  if(geometry().intersect(cursor.pos())) return true;

  return false;
}

void LabelFrame::paint(VertexPainter& painter, Geometry parent)
{ 
  const Style& style = ownStyle();
  auto& font = *style.font;

  Geometry g = geometry();
  auto caption_sz = m_caption.size();

  vec2 center = g.center();
  center *= 0.5f;

  auto pipeline = gx::Pipeline()
    .alphaBlend()
    .scissor(ui().scissorRect(parent.clip(g)))
    .primitiveRestart(Vertex::RestartIndex)
    ;

  painter
    .pipeline(pipeline)
    .rect(g.expand(1), m_bg)
    ;

  if(gravity() == Center) {
    painter.drawableCentered(m_caption, g);
  } else {
    switch(gravity()) {
    case Left:   painter.drawable(m_caption, { g.x, center.y }); break;
    case Right:  painter.drawable(m_caption, { g.x + (g.w-caption_sz.x), center.y }); break;
    }
  }
}

LabelFrame& LabelFrame::caption(const std::string& caption)
{
  m_caption = ui().drawable().fromText(ownStyle().font, caption, white());

  return *this;
}

LabelFrame& LabelFrame::background(Color bg)
{
  m_bg = bg;

  return *this;
}

vec2 LabelFrame::sizeHint() const
{
  return m_caption.size();
}

}