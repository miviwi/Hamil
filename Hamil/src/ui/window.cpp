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
      auto mouse_over = decorationsGeometry().intersect(cursor.pos());
      if(!mouse_over) return false;

      position(geometry().pos() + vec2{ mouse->dx, mouse->dy });
      return true;
    }
  }

  return handled;
}

void WindowFrame::paint(VertexPainter& painter, Geometry parent)
{
  const Style& style = ui().style();

  Geometry g = geometry();

  auto pipeline = gx::Pipeline()
    .alphaBlend()
    .scissor(ui().scissorRect(parent.clip(g)))
    .primitiveRestart(Vertex::RestartIndex)
    ;

  painter
    .pipeline(pipeline)
    .rect(decorationsGeometry(), white())
    .rect(g, black().opacity(0.4));

  if(!m_content) return;

  m_content->paint(painter, g);
}

Frame& WindowFrame::position(vec2 pos)
{
  const auto& style = ui().style();
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
  const auto& style = ui().style();
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