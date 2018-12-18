#include <ui/window.h>

namespace ui {

WindowFrame::~WindowFrame()
{
  delete m_content;
}

bool WindowFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  // If moving don't pass input to the content Frame
  bool handled = m_state == Moving ? false : m_content->input(cursor, input);
  if(!handled) {
    auto mouse = input->get<win32::Mouse>();
    if(!mouse) return false;

    using win32::Mouse;
    if(mouse->buttonDown(Mouse::Left)) {
      bool mouse_over_decorations = decorationsGeometry().intersect(cursor.pos());

      if(mouse_over_decorations) m_state = Moving;

      ui().capture(this);
    } else if(mouse->buttonUp(Mouse::Left)) {
      m_state = Default;
      ui().capture(nullptr);
    }

    if(m_state == Moving && mouse->buttons & Mouse::Left) {
      position(geometry().pos() + vec2{ mouse->dx, mouse->dy });
    }
  }

  return handled || m_state != Default;
}

void WindowFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = ownStyle();

  Geometry g = geometry(),
    clipped_g = parent.clip(g);

  Geometry decor_g = decorationsGeometry();
  uint decor_corners = VertexPainter::TopLeft|VertexPainter::TopRight;
  Color decor_color = style.bg.color[2].darkenf(0.1);

  Color bg = m_bg ? *m_bg : style.window.bg;

  auto pipeline = painter.defaultPipeline(ui().scissorRect(clipped_g));

  painter
    .pipeline(pipeline)
    .roundedRect(g, style.window.radius, VertexPainter::All, bg)
    .roundedRect(decor_g, style.window.radius, decor_corners, decor_color)
    .roundedBorder(g, style.window.radius, VertexPainter::All, decor_color.darkenf(0.2))
    .drawableCentered(m_title, decor_g);

  if(!m_content) return;

  m_content->paint(painter, g);
}

WindowFrame& WindowFrame::title(const std::string& title)
{
  m_title = ui().drawable().fromText(ownStyle().font, title, white());

  return *this;
}

WindowFrame& WindowFrame::background(Color c)
{
  m_bg = c;

  return *this;
}

WindowFrame& WindowFrame::content(Frame *content)
{
  m_content = content;

  m_content->attached(this);

  vec2 content_pos = m_content->positionRelative();
  m_content->position(content_pos + DecorationsSize + ownStyle().window.margin);

  return *this;
}

WindowFrame& WindowFrame::content(Frame& content_)
{
  return content(&content_);
}

vec2 WindowFrame::sizeHint() const
{
  const auto& style = ownStyle();
  const auto& window = style.window;

  auto content_size = m_content ? m_content->size() : vec2();

  return content_size + DecorationsSize + window.margin*2.0f;
}

Geometry WindowFrame::decorationsGeometry() const
{
  auto g = geometry();

  return {
    g.x, g.y,
    g.w, DecorationsSize.y
  };
}

}