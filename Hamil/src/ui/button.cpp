#include <ui/button.h>

#include <cmath>

namespace ui {

ButtonFrame::~ButtonFrame()
{
}

bool ButtonFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  bool mouse_over = getSolidGeometry().intersect(cursor.pos());
  if(!mouse_over && m_state != Pressed) {
    ui().capture(nullptr);
    return false;
  }

  if(m_state == Default) {
    m_state = Hover;
    ui().capture(this);
  }

  auto mouse = input->get<win32::Mouse>();
  if(!mouse) return false;

  using win32::Mouse;
  if(mouse->buttonDown(Mouse::Left)) {
    m_state = Pressed;
    ui().keyboard(nullptr);
  } else if(m_state == Pressed && mouse->buttonUp(Mouse::Left)) {
    if(mouse_over) {
      m_state = Hover;

      emitClicked();
    } else {
      ui().capture(nullptr);
    }
  }

  return true;
}

void ButtonFrame::losingCapture()
{
  m_state = Default;
}

void ButtonFrame::captionPaint(const Drawable& caption, VertexPainter& painter,
  Geometry parent, State state)
{
  const Style& style = ownStyle();
  const auto& button = style.button;

  auto margin = button.margin;

  Geometry g = geometry().contract(margin);

  auto luminance = button.color[1].luminance().r;
  byte factor = 0;
  switch(state) {
  case Default: factor = 0; break;
  case Hover:   factor = luminance/4; break;
  case Pressed: factor = luminance; break;
  }

  Color color[] = {
    button.color[0].lighten(factor),
    button.color[1].lighten(factor),
  };

  Geometry highlight_g ={
    g.x + g.w*0.02f, g.y + g.h*0.08f,
    g.w*0.96f, g.h*0.5f
  };

  auto pipeline = gx::Pipeline()
    .premultAlphaBlend()
    .scissor(ui().scissorRect(parent.clip({ g.x, g.y, g.w+1, g.h+1 })))
    .primitiveRestart(Vertex::RestartIndex)
    ;

  painter
    .pipeline(pipeline)
    .roundedRect(g, button.radius, VertexPainter::All, color[0])
    .roundedRect(highlight_g, button.radius, VertexPainter::All, color[1])
    .roundedBorder(g, button.radius, VertexPainter::All, black())
    .drawableCentered(caption, g)
    ;
}

vec2 ButtonFrame::captionSizeHint(const Drawable& caption) const
{
  auto font = ownStyle().font;
  float font_height = font->height();

  return {
    std::max(caption.size().x+20, 50.0f),
    std::max(caption.size().y, font_height) + font_height*0.8f
  };
}

void PushButtonFrame::paint(VertexPainter& painter, Geometry parent)
{
  captionPaint(m_caption, painter, parent, m_state);
}

PushButtonFrame& PushButtonFrame::caption(std::string caption)
{
  m_caption = ui().drawable().fromText(ownStyle().font, caption, white());

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
  return captionSizeHint(m_caption);
}

Geometry PushButtonFrame::getSolidGeometry() const
{
  return geometry();
}

void PushButtonFrame::emitClicked()
{
  m_on_click.emit(this);
}

void ToggleButtonFrame::paint(VertexPainter& painter, Geometry parent)
{
  captionPaint(m_caption, painter, parent, m_value ? Pressed : m_state);
}

ToggleButtonFrame& ToggleButtonFrame::caption(std::string caption)
{
  m_caption = ui().drawable().fromText(ownStyle().font, caption, white());

  return *this;
}

ToggleButtonFrame& ToggleButtonFrame::value(bool value)
{
  m_value = value;

  return *this;
}

bool ToggleButtonFrame::value() const
{
  return m_value;
}

ToggleButtonFrame& ToggleButtonFrame::onClick(OnClick::Slot on_click)
{
  m_on_click.connect(on_click);

  return *this;
}

ToggleButtonFrame::OnClick& ToggleButtonFrame::click()
{
  return m_on_click;
}

vec2 ToggleButtonFrame::sizeHint() const
{
  return captionSizeHint(m_caption);
}

Geometry ToggleButtonFrame::getSolidGeometry() const
{
  return geometry();
}

void ToggleButtonFrame::emitClicked()
{
  m_value = !m_value;

  m_on_click.emit(this);
}

void CheckBoxFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = ownStyle();
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
    .scissor(ui().scissorRect(parent.clip(g)))
    .premultAlphaBlend()
    .primitiveRestart(Vertex::RestartIndex)
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