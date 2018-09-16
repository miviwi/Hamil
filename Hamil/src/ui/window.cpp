#include <ui/window.h>

namespace ui {

WindowFrame::~WindowFrame()
{
  delete m_content;
}

bool WindowFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  bool handled = m_content->input(cursor, input);
  if(!handled) {
    auto mouse = input->get<win32::Mouse>();
    if(!mouse) return false;

    using win32::Mouse;
    if(mouse->buttons & Mouse::Left) {
      Geometry g = geometry();

      bool mouse_over_decorations = decorationsGeometry().intersect(cursor.pos());
      bool mouse_over = g.intersect(cursor.pos());

      if(mouse_over_decorations) {
        position(geometry().pos() + vec2{ mouse->dx, mouse->dy });
      }

      return mouse_over;
    }
  }

  return handled;
}

void WindowFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = ownStyle();

  Geometry g = geometry();

  auto pipeline = gx::Pipeline()
    .alphaBlend()
    .scissor(ui().scissorRect(parent.clip(g)))
    .primitiveRestart(Vertex::RestartIndex)
    ;

  painter
    .pipeline(pipeline)
    .roundedRect(decorationsGeometry(), style.window.radius, VertexPainter::TopLeft|VertexPainter::TopRight, style.bg.color[2])
    .roundedRect(g, style.window.radius, VertexPainter::All, black().opacity(0.4));

  if(!m_content) return;

  m_content->paint(painter, g);
}

Frame& WindowFrame::position(vec2 pos)
{
  const auto& style = ownStyle();
  const auto& window = style.window;

  Frame::position(pos);
  if(m_content) m_content->position(pos + DecorationsSize + window.margin);

  return *this;
}

WindowFrame& WindowFrame::content(Frame *content)
{
  m_content = content;

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

  auto content_size = m_content ? m_content->geometry().size() : vec2();

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