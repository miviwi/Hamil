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
  auto& font = *ownFont();

  Geometry g = geometry();
  auto caption_sz = m_caption.size();

  vec2 center = g.center();

  auto pipeline = gx::Pipeline()
    .premultAlphaBlend()
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
    float y = 0.0f;
    if(m_caption.type() == Drawable::Text) {
      y = center.y - (caption_sz.y - font.height())/2.0f - font.descender();
    } else {
      y = center.y - caption_sz.y/2.0f;
    }

    switch(gravity()) {
    case Left:   painter.drawable(m_caption, { g.x, y }); break;
    case Right:  painter.drawable(m_caption, { g.x + (g.w-caption_sz.x), y }); break;
    }
  }
}

LabelFrame& LabelFrame::caption(const std::string& caption)
{
  m_caption = ui().drawable().fromText(ownFont(), caption, m_color);

  return *this;
}

LabelFrame& LabelFrame::drawable(const Drawable& img)
{
  m_caption = img;

  return *this;
}

LabelFrame& LabelFrame::font(const ft::Font::Ptr& font)
{
  m_font = font;

  return *this;
}

LabelFrame& LabelFrame::color(Color c)
{
  m_color = c;

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

ft::Font::Ptr LabelFrame::ownFont()
{
  return m_font ? m_font : ownStyle().font;
}

}