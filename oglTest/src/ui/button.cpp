#include <ui/button.h>
#include <ui/style.h>
#include <ui/painter.h>
#include <gx/pipeline.h>

#include <cmath>

#include <Windows.h>

namespace ui {

ButtonFrame::~ButtonFrame()
{
}

bool ButtonFrame::input(ivec2 mouse_pos, const InputPtr& input)
{
  bool mouse_over = getSolidGeometry().intersect(mouse_pos);
  if(!mouse_over && m_state != Pressed) {
    m_ui->capture(nullptr);
    return false;
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
    if(mouse_over) {
      m_state = Hover;

      emitClicked();
    } else {
      m_ui->capture(nullptr);
    }
  }

  return true;
}

void ButtonFrame::losingCapture()
{
  m_state = Default;
}

void PushButtonFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = m_ui->style();
  const auto& button = style.button;

  auto margin = button.margin;

  Geometry g = geometry().contract(margin);

  auto luminance = button.color[1].luminance().r;
  byte factor = 0;
  switch(m_state) {
  case Default: factor = 0; break;
  case Hover:   factor = luminance/4; break;
  case Pressed: factor = luminance; break;
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
    .scissor(Ui::scissor_rect(parent.clip({ g.x, g.y, g.w+1, g.h+1 })))
    .primitiveRestart(0xFFFF)
    ;

  painter
    .pipeline(pipeline)
    .roundedRect(g, button.radius, VertexPainter::All, color[0])
    .roundedRect(highlight_g, button.radius, VertexPainter::All, color[1])
    .roundedBorder(g, button.radius, VertexPainter::All, black())
    .textCentered(*style.font, m_caption, g, white())
    ;
}

PushButtonFrame& PushButtonFrame::caption(std::string caption)
{
  m_caption = std::move(caption);

  return *this;
}

PushButtonFrame& PushButtonFrame::onClick(OnClick::Slot on_click)
{
  m_on_click.connect(on_click);

  return *this;
}

PushButtonFrame::OnClick& PushButtonFrame::click()
{
  return m_on_click;
}

vec2 PushButtonFrame::sizeHint() const
{
  const ft::Font& font = *m_ui->style().font;
  auto s = font.stringMetrics(m_caption);

  float font_height = font.height();

  return {
    std::max(font.width(s)+20, 110.0f),
    font.height(s) + font_height*0.8f
  };
}

Geometry PushButtonFrame::getSolidGeometry() const
{
  return geometry();
}

void PushButtonFrame::emitClicked()
{
  m_on_click.emit(this);
}

void CheckBoxFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = m_ui->style();
  const auto& button = style.button;

  Geometry g = geometry();
  vec2 center = g.center();

  Geometry box = getSolidGeometry();

  auto luminance = button.color[1].luminance().r;
  byte factor = 0;
  switch(m_state) {
  case Default: factor = 0; break;
  case Hover:   factor = luminance/2; break;
  case Pressed: factor = luminance; break;
  }

  Color color[] = {
    button.color[0].lighten(factor),
    button.color[1].lighten(luminance * (m_state == Pressed ? 4 : 2)),
  };

  auto pipeline = gx::Pipeline()
    .scissor(Ui::scissor_rect(parent.clip(g)))
    .alphaBlend()
    .primitiveRestart(0xFFFF)
    ;

  painter
    .pipeline(pipeline)
    .rect(box, color[0])
    ;

  if(m_value) {
    vec2 a = {
      box.x + PixelMargin,
      box.y + PixelMargin
    },
      b = {
      box.x+box.w - PixelMargin,
      box.y+box.h - PixelMargin
    };

    painter
      .line(a, b, 3, VertexPainter::CapButt, color[1], color[1])
      .line({ b.x, a.y }, { a.x, b.y }, 3.5, VertexPainter::CapButt, color[1], color[1])
      ;
  }

  painter.border(box, 1, black());
}

CheckBoxFrame& CheckBoxFrame::value(bool value)
{
  m_value = value;

  return *this;
}

bool CheckBoxFrame::value() const
{
  return m_value;
}

CheckBoxFrame& CheckBoxFrame::onClick(OnClick::Slot on_click)
{
  m_on_click.connect(on_click);

  return *this;
}

CheckBoxFrame::OnClick& CheckBoxFrame::click()
{
  return m_on_click;
}

vec2 CheckBoxFrame::sizeHint() const
{
  return { Dimensions*2, Dimensions*1.2f };
}

Geometry CheckBoxFrame::getSolidGeometry() const
{
  vec2 center = geometry().center();

  return Geometry{
    center.x - Dimensions/2.0f, center.y - Dimensions/2.0f,
    Dimensions, Dimensions
  };
}

void CheckBoxFrame::emitClicked()
{
  m_value = !m_value;

  m_on_click.emit(this);
}

}