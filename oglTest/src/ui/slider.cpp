#include "ui/slider.h"

namespace ui {

SliderFrame::~SliderFrame()
{
}

bool SliderFrame::input(ivec2 mouse_pos, const InputPtr& input)
{
  bool mouse_inside = geometry().intersect(mouse_pos);
  if(!mouse_inside && m_state != Pressed) {
    m_ui->capture(nullptr);
    return false;
  }

  vec2 m_pos = { (float)mouse_pos.x, (float)mouse_pos.y };
  bool over_head = headPos().distance(m_pos) < m_ui->style().slider.width*1.05f;

  if(m_state != Pressed) m_state = over_head ? Hover : Default;

  if(input->getTag() != win32::Mouse::tag()) return false;

  using win32::Mouse;
  auto mouse = (win32::Mouse *)input.get();
  if(mouse->buttonDown(Mouse::Left)) {
    if(over_head) {
      m_state = Pressed;
      m_ui->capture(this);
    }
  } else if(mouse->buttonUp(Mouse::Left)) {
    if(mouse_inside) {
      m_state = Hover;
    } else {
      m_state = Default;
      m_ui->capture(nullptr);
    }
  } 
  
  if(m_state == Pressed && mouse->buttons & Mouse::Left) {
    m_value += step()*(double)mouse->dx;
    m_value = clampedValue(m_value);

    m_on_change.emit(this);

    return true;
  }

  return false;
}

void SliderFrame::losingCapture()
{
  m_state = Default;
}

SliderFrame& SliderFrame::range(double min, double max)
{
  m_min = min; m_max = max;
  m_value = (max+min)/2.0;

  return *this;
}

SliderFrame& SliderFrame::value(double value)
{
  m_value = clampedValue(value);
  m_on_change.emit(this);

  return *this;
}

SliderFrame& SliderFrame::onChange(OnChange::Slot on_change)
{
  m_on_change.connect(on_change);

  return *this;
}

double SliderFrame::value() const
{
  return m_value;
}

int SliderFrame::ivalue() const
{
  return (int)m_value;
}

SliderFrame::OnChange& SliderFrame::change()
{
  return m_on_change;
}

double SliderFrame::clampedValue(double value)
{
  return clamp(value, m_min, m_max);
}

void HSliderFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = m_ui->style();
  const auto& slider = style.slider;

  Geometry g = geometry();

  float w = width();
  vec2 center = g.center();

  float value_factor = (float)(m_value-m_min)/(float)(m_max-m_min);

  vec2 pos[2] = {
    { g.x + g.w*Margin, center.y },
    { g.x + g.w*Margin + w, center.y },
  };

  vec2 head_pos = headPos();
  vec2 limit[2] = {
    { pos[0].x + w*0.08f - slider.width , center.y },
    head_pos
  };

  auto luminance = slider.color[1].luminance().r;
  byte factor = 0;
  switch(m_state) {
  case Default: factor = 0; break;
  case Hover:   factor = luminance/2; break;
  case Pressed: factor = luminance*2; break;
  }

  Color head_color[2] = {
    slider.color[0].lighten(factor),
    slider.color[1].lighten(factor),
  };

  const float highlight_r = slider.width/1.5f;
  vec2 highlight_pos = {
    head_pos.x + highlight_r/2.3f,
    head_pos.y - highlight_r/2.3f
  };

  Color value_color = slider.color[1].lighten(luminance);

  auto pipeline = gx::Pipeline()
    .alphaBlend()
    .scissor(Ui::scissor_rect(parent.clip(g)))
    .primitiveRestart(0xFFFF)
    ;

  painter
    .pipeline(pipeline)
    .line(pos[0], pos[1], slider.width*0.9f, VertexPainter::CapRound, slider.color[0], slider.color[0])
    .line(limit[0], limit[1], slider.width*0.4f, VertexPainter::CapRound, value_color, value_color)
    .lineBorder(pos[0], pos[1], slider.width*0.9f, VertexPainter::CapRound, black(), black())
    .circle(head_pos, slider.width, head_color[0])
    .circle(highlight_pos, highlight_r, head_color[1])
    .arcFull(head_pos, slider.width, black())
    ;
}

vec2 HSliderFrame::sizeHint() const
{
  const auto& slider = m_ui->style().slider;

  return { 150, slider.width*3 };
}

float HSliderFrame::width() const
{
  return geometry().w * (1.0f - 2.0f*Margin);
}

float HSliderFrame::innerWidth() const
{
  return width() * (1.0f - 2.0f*InnerMargin);
}

double HSliderFrame::step() const
{
  return (m_max-m_min)/(width() * (1 - 2.0f*InnerMargin));
}

vec2 HSliderFrame::headPos() const
{
  Geometry g = geometry();
  vec2 center = g.center();

  float w = width();
  float value_factor = (float)(m_value-m_min)/(float)(m_max-m_min);

  return vec2{
    (g.x + g.w*Margin) + w*InnerMargin + value_factor*innerWidth(),
    center.y
  };
}

}