#include <ui/frame.h>
#include <ui/painter.h>

namespace ui {

Frame::Frame(Ui &ui, const char *name, Geometry geom) :
  m_ui(&ui), m_name(name), m_gravity(Center), m_geom(geom), m_pad({ 0.0f, 0.0f })
{
  m_ui->registerFrame(this);
}

Frame::Frame(Ui& ui, Geometry geom) :
  Frame(ui, nullptr, geom)
{
}

Frame::Frame(Ui& ui, const char *name) :
  Frame(ui, name, Geometry{ 0, 0, 0, 0 })
{
}

Frame::Frame(Ui& ui) :
  Frame(ui, nullptr)
{
}

Frame::~Frame()
{
}

bool Frame::input(CursorDriver& cursor, const InputPtr& input)
{
  if(!m_geom.intersect(cursor.pos())) return false;

  if(auto mouse = input->get<win32::Mouse>()) {
    using win32::Mouse;

    if(mouse->buttonDown(Mouse::Left)) {
      //m_countrer++;
    } else if(mouse->buttons & Mouse::Left) {
      m_geom.x += mouse->dx; m_geom.y += mouse->dy;
    }

    return true;
  }

  return false;
}

Frame& Frame::geometry(Geometry g)
{
  m_geom = {
    floor(g.x), floor(g.y),
    ceil(g.w), ceil(g.h)
  };

  return *this;
}

Geometry Frame::geometry() const
{
  Geometry g = m_geom;
  if(!g.w && !g.h) {
    vec2 size = sizeHint() + m_pad*2.0f;
    return {
      g.x, g.y,
      size.x, size.y
    };
  }

  return g;
}

Frame& Frame::gravity(Gravity gravity)
{
  m_gravity = gravity;

  return *this;
}

Frame::Gravity Frame::gravity() const
{
  return m_gravity;
}

void Frame::losingCapture()
{
}

void Frame::attached()
{
}

vec2 Frame::sizeHint() const
{
  return { 0, 0 };
}

Ui& Frame::ui()
{
  return *m_ui;
}

const Ui& Frame::ui() const
{
  return *m_ui;
}

const Style& Frame::ownStyle() const
{
  return ui().style();
}

bool Frame::evMouseEnter(const MouseMoveEvent& e)
{
  return false;
}

bool Frame::evMouseLeave(const MouseMoveEvent& e)
{
  return false;
}

bool Frame::evMouseMove(const MouseMoveEvent& e)
{
  return false;
}

bool Frame::evMouseDown(const MouseButtonEvent& e)
{
  return false;
}

bool Frame::evMouseUp(const MouseButtonEvent& e)
{
  return false;
}

bool Frame::evMouseDrag(const MouseDragEvent& e)
{
  return false;
}

Frame& Frame::position(vec2 pos)
{
  m_geom.x = pos.x;
  m_geom.y = pos.y;

  return *this;
}

vec2 Frame::padding() const
{
  return m_pad;
}

Frame& Frame::padding(vec2 pad)
{
  m_pad = pad;

  return *this;
}

void Frame::paint(VertexPainter& painter, Geometry parent)
{
}

}