#include <ui/frame.h>
#include <ui/painter.h>

namespace ui {

Frame::Frame(Ui &ui, const char *name, Geometry geom) :
  m_parent(nullptr),
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
  return m_geom.intersect(cursor.pos());
}

Frame& Frame::geometry(Geometry g)
{
  m_geom = {
    g.pos().floor(),
    g.size().ceil()
  };

  return *this;
}

Geometry Frame::geometry() const
{
  return Geometry(position(), size());
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

void Frame::attached(Frame *parent)
{
  m_parent = parent;
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

vec2 Frame::position() const
{
  return positionRelative() + (m_parent ? m_parent->position() : vec2());
}

vec2 Frame::positionRelative() const
{
  return m_geom.pos();
}

Frame& Frame::position(vec2 pos)
{
  m_geom.x = pos.x;
  m_geom.y = pos.y;

  return *this;
}

vec2 Frame::size() const
{
  vec2 sz = m_geom.size();
  vec2 pad = padding();

  // If no explicit size was given use the sizeHint()
  if(sz.isZero()) sz = sizeHint();

  return vec2::max(sz, pad);
}

Frame& Frame::size(vec2 sz)
{
  m_geom.w = sz.x;
  m_geom.h = sz.y;

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