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
  if(!m_geom.contract(5).intersect(mouse_pos)) return m_depressed = false;

  if(input->getTag() != win32::Mouse::tag()) return false;

  using win32::Mouse;
  auto mouse = (win32::Mouse *)input.get();
  if(mouse->buttonDown(Mouse::Left)) {
    m_depressed = true;
  } else if(m_depressed && mouse->buttonUp(Mouse::Left)) {
    m_depressed = false;

    if(m_on_click) m_on_click(this);
  }

  return true;
}

void ButtonFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = m_ui->style();
  Geometry g = parent.clip(m_geom).contract(1);

  auto half_luminance = style.button.color[1].luminance().r / 2;

  Color color[2] = {
    m_depressed ? style.button.color[0].darken(half_luminance) : style.button.color[0],
    m_depressed ? style.button.color[1].darken(half_luminance) : style.button.color[1],
  };

  painter
    .pipeline(gx::Pipeline()
              .alphaBlend()
              .scissor(Ui::scissor_rect(g)))
    .roundedRect(g, style.button.radius, VertexPainter::All, color[1], color[1])
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