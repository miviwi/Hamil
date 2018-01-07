#include "ui/button.h"
#include "ui/style.h"
#include "ui/painter.h"
#include "pipeline.h"

#include <cmath>

#include <Windows.h>

namespace ui {

ButtonFrame::~ButtonFrame()
{
}

bool ButtonFrame::input(ivec2 mouse_pos, const InputPtr& input)
{
  if(!m_geom.intersect(mouse_pos)) return m_state = Default;

  if(m_state == Default) m_state = Hover;

  if(input->getTag() != win32::Mouse::tag()) return false;

  using win32::Mouse;
  auto mouse = (win32::Mouse *)input.get();
  if(mouse->buttonDown(Mouse::Left)) {
    m_state = Pressed;
  } else if(m_state == Pressed && mouse->buttonUp(Mouse::Left)) {
    m_state = Hover;

    if(m_on_click) m_on_click(this);
  } else if(mouse->event == Mouse::Move) {
    if(mouseWillLeave(mouse_pos, mouse)) m_state = Default;

    return false;
  }

  return true;
}

void ButtonFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = m_ui->style();
  Geometry g = parent.clip(m_geom).contract(style.button.margin);

  auto half_luminance = style.button.color[1].luminance().r / 2;

  byte factor = 0;
  switch(m_state) {
  case Default: factor = 0; break;
  case Hover:   factor = half_luminance/4; break;
  case Pressed: factor = half_luminance; break;
  }

  Color color[] = {
    style.button.color[0].darken(factor),
    style.button.color[1].darken(factor),
  };

  Geometry highlight_g = {
    g.x + g.w*0.01f, g.y + g.h*0.1f,
    g.w*0.98f, g.h*0.5f
  };

  painter
    .pipeline(gx::Pipeline()
              .alphaBlend()
              .scissor(Ui::scissor_rect(g)))
    .roundedRect(g, style.button.radius, VertexPainter::All, color[0], color[0])
    .roundedRect(highlight_g, style.button.radius, VertexPainter::All, color[1], color[1])
    .roundedBorder(g, style.button.radius, VertexPainter::All, black())
    .textCentered(*style.font, m_caption, g, white())
    ;
}

ButtonFrame& ButtonFrame::caption(std::string caption)
{
  m_caption = m_ui->style().font->string(caption.c_str());

  return *this;
}

ButtonFrame& ButtonFrame::onClick(OnClickFn on_click)
{
  m_on_click = on_click;

  return *this;
}

}