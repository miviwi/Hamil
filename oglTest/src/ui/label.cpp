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
  const Style& style = m_ui->style();
  auto& font = *style.font;

  Geometry g = geometry();
  vec2 center = g.center();

  center.y -= style.font->descender();

  auto pipeline = gx::Pipeline()
    .alphaBlend()
    .scissor(m_ui->scissorRect(parent.clip(g)))
    .primitiveRestart(Vertex::RestartIndex)
    ;

  painter
    .pipeline(pipeline)
    ;

  if(gravity() == Center) {
    painter.textCentered(m_caption, g);
  } else {
    switch(gravity()) {
    case Left:   painter.text(m_caption, { g.x, center.y }); break;
    case Right:  painter.text(m_caption, { g.x + (g.w-m_caption.size().x), center.y }); break;
    }
  }
}

LabelFrame& LabelFrame::caption(const std::string& caption)
{
  m_caption = m_ui->drawable().fromText(m_ui->style().font, caption, white());

  return *this;
}

vec2 LabelFrame::sizeHint() const
{
  return m_caption.size();
}

}