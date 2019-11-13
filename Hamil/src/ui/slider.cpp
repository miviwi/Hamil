#include <ui/slider.h>

namespace ui {

SliderFrame::~SliderFrame()
{
}

bool SliderFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  bool mouse_inside = geometry().intersect(cursor.pos());
  if(!mouse_inside && m_state != Pressed) {
    ui().capture(nullptr);
    return false;
  }

  bool over_head = headPos().distance(cursor.pos()) < ownStyle().slider.width*1.05f;

  if(m_state != Pressed) m_state = over_head ? Hover : Default;

  auto mouse = input->get<os::Mouse>();
  if(!mouse) return false;

  using os::Mouse;
  if(mouse->buttonDown(Mouse::Left)) {
    m_state = Pressed;
    ui().capture(this);
    ui().keyboard(nullptr);
  } else if(mouse->buttonUp(Mouse::Left)) {
    if(mouse_inside) {
      m_state = Hover;
    } else {
      m_state = Default;
      ui().capture(nullptr);
    }
  } 
  
  if(m_state == Pressed && mouse->buttons & Mouse::Left) {
    m_value = clickedValue(cursor.pos() + vec2{ mouse->dx, mouse->dy });
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

SliderFrame& SliderFrame::step(double step)
{
  m_step = step;

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

SliderFrame& SliderFrame::valuef(double fract)
{
  return value(lerp(m_min, m_max, fract));
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

double SliderFrame::valueDelta() const
{
  return m_max - m_min;
}

double SliderFrame::valueFactor() const
{
  return (m_value-m_min)/valueDelta();
}

double SliderFrame::stepQuantize(double fine_value) const
{
  return fine_value - fmod(fine_value, m_step);
}

double SliderFrame::clampedValue(double value) const
{
  return clamp(value, m_min, m_max);
}

void HSliderFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = ownStyle();
  const auto& slider = style.slider;

  Geometry g = geometry(),
    clipped_g = parent.clip(g);

  float w = width();
  vec2 center = g.center();

  float value_factor = (float)valueFactor();

  vec2 pos[2] = {
    { g.x + g.w*Margin, center.y },
    { g.x + g.w*Margin + w, center.y },
  };

  vec2 head_pos = headPos();
  vec2 limit[2] = {
    { pos[0].x + w*0.08f - slider.width , center.y },
    head_pos
  };

  double factor = 0.0;
  switch(m_state) {
  case Hover:   factor = 0.1; break;
  case Pressed: factor = 0.4; break;
  }

  Color head_color[2] = {
    slider.color[0].lightenf(factor),
    slider.color[1].lightenf(factor),
  };

  const float highlight_r = slider.width/1.5f;
  vec2 highlight_pos = {
    head_pos.x + highlight_r/2.3f,
    head_pos.y - highlight_r/2.3f
  };

  Color value_color = slider.color[1].lightenf(0.1);

  auto pipeline = painter.defaultPipeline(ui().scissorRect(clipped_g));

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
  const auto& slider = ownStyle().slider;

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
  return stepQuantize(valueDelta()/innerWidth());
}

vec2 HSliderFrame::headPos() const
{
  Geometry g = geometry();
  vec2 center = g.center();

  float w = width();
  float value_factor = (float)valueFactor();

  return vec2{
    (g.x + g.w*Margin) + w*InnerMargin + value_factor*innerWidth(),
    center.y
  };
}

double HSliderFrame::clickedValue(vec2 pos) const
{
  Geometry g = geometry();
  float x = pos.x - (g.x + g.w*(Margin+InnerMargin));

  double fine_value = lerp(m_min, m_max, x/innerWidth());

  return clampedValue(stepQuantize(fine_value));
}

}
