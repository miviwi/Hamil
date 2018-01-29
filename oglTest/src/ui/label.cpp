#include <ui/label.h>

namespace ui {

LabelFrame::~LabelFrame()
{
}

bool LabelFrame::input(ivec2 mouse_pos, const InputPtr& input)
{
  if(geometry().intersect(mouse_pos)) return true;

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
    .scissor(Ui::scissor_rect(parent.clip(g)))
    .primitiveRestart(0xFFFF)
    ;

  painter
    .pipeline(pipeline)
    ;

  if(gravity() == Center) {
    painter.textCentered(font, m_caption, g, white());
  } else {
    auto s = font.stringMetrics(m_caption);

    switch(gravity()) {
    case Left:   painter.text(font, m_caption, { g.x, center.y }, white()); break;
    case Right:  painter.text(font, m_caption, { g.x + (g.w-font.width(s)), center.y }, white()); break;
    }
  }
}

LabelFrame& LabelFrame::caption(const std::string& caption)
{
  m_caption = caption;

  return *this;
}

const std::string& LabelFrame::caption() const
{
  return m_caption;
}

vec2 LabelFrame::sizeHint() const
{
  const ft::Font& font = *m_ui->style().font;
  auto s = font.stringMetrics(m_caption);

  return { font.width(s), font.height(s) };
}

}