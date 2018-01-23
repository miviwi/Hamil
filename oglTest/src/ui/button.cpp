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
  bool mouse_inside = m_geom.intersect(mouse_pos);
  if(!mouse_inside && m_state != Pressed) {
    m_ui->capture(nullptr);
    return m_state = Default;
  }

  if(m_state == Default) {
    m_state = Hover;
    m_ui->capture(this);
  }

  if(input->getTag() != win32::Mouse::tag()) return false;

  using win32::Mouse;
  auto mouse = (win32::Mouse *)input.get();
  if(mouse->buttonDown(Mouse::Left)) {
    m_state = Pressed;
  } else if(m_state == Pressed && mouse->buttonUp(Mouse::Left)) {
    if(mouse_inside) {
      m_state = Hover;

      m_on_click.emit(this);
    } else {
      m_state = Default;
      m_ui->capture(nullptr);
    }
  }

  return true;
}

void ButtonFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = m_ui->style();
  const auto& button = style.button;

  Geometry g = parent.clip(m_geom).contract(button.margin);

  auto half_luminance = button.color[1].luminance().r / 2;

  byte factor = 0;
  switch(m_state) {
  case Default: factor = 0; break;
  case Hover:   factor = half_luminance/4; break;
  case Pressed: factor = half_luminance; break;
  }

  Color color[] = {
    button.color[0].lighten(factor),
    button.color[1].lighten(factor),
  };

  Geometry highlight_g = {
    g.x + g.w*0.02f, g.y + g.h*0.08f,
    g.w*0.96f, g.h*0.5f
  };

  auto pipeline = gx::Pipeline()
    .alphaBlend()
    .scissor(Ui::scissor_rect(g))
    .primitiveRestart(0xFFFF)
    ;

  painter
    .pipeline(pipeline)
    .roundedRect(g, button.radius, VertexPainter::All, color[0], color[0])
    .roundedRect(highlight_g, button.radius, VertexPainter::All, color[1], color[1])
    .roundedBorder(g, button.radius, VertexPainter::All, black())
    .textCentered(*style.font, m_caption, g, white())
    ;
}

void ButtonFrame::losingCapture()
{
  m_state = Default;
}

ButtonFrame& ButtonFrame::caption(std::string caption)
{
  m_caption = std::move(caption);

  return *this;
}

ButtonFrame& ButtonFrame::onClick(ButtonFrame::OnClick::Slot on_click)
{
  m_on_click.connect(on_click);

  return *this;
}

ButtonFrame::OnClick& ButtonFrame::click()
{
  return m_on_click;
}

}