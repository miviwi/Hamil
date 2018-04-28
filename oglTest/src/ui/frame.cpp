#include <ui/frame.h>
#include <ui/painter.h>

namespace ui {

Frame::Frame(Ui &ui, const char *name, Geometry geom) :
  m_ui(&ui), m_name(name), m_gravity(Center), m_geom(geom),
  m_animation({
      make_animation_channel({
            keyframe(Color(255, 0, 0), 2.0f),
            keyframe(Color(0, 255, 0), 2.0f),
            keyframe(Color(0, 0, 255), 2.0f),
            keyframe(Color(0, 255, 0), 2.0f),
      }, EaseQuinticOut, RepeatLoop),

      make_animation_channel({
            keyframe(0.0f, 3.0f),
            keyframe(-150.0f, 3.0f),
      }, EaseBounce, RepeatLoop)
  })
{
  m_ui->registerFrame(this);
  m_animation.start();
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
    vec2 size = sizeHint();
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

vec2 Frame::sizeHint() const
{
  return { 0, 0 };
}

void Frame::paint(VertexPainter& painter, Geometry parent)
{
  Geometry g = m_geom;
  auto style = m_ui->style();

  auto ga = vec2{ g.x, g.y },
    gb = vec2{ g.x+g.w, g.y+g.h };

  vec2 circle = g.center();
  vec2 c = g.center();

  auto time = GetTickCount();
  auto anim_time = 10000;

  auto anim_factor = (float)(time % anim_time)/(float)anim_time;

  float radius = lerp(0.0f, 55.0f/2.0f, anim_factor);

  Color gray = { 100, 100, 100, 255 },
    dark_gray = { 200, 200, 200, 255 };
  Geometry bg_pos = { gb.x - 30.0f, ga.y, 30.0f, 30.0f };
  vec2 close_btn = {
    (bg_pos.w / 2.0f) + bg_pos.x,
    (bg_pos.h / 2.0f) + bg_pos.y,
  };

  Geometry round_rect = {
    g.x+15.0f, g.y+15.0f, 55.0f, 55.0f
  };

  vec2 title_pos = {
    g.x+5.0f, g.y+style.font->ascender()
  };

  const char *title = "ui::Frame title goes here";

  float angle = lerp(0.0f, 2.0f*PIf, anim_factor);

  vec2 line_delta = {
    cos(angle)*50.0f, sin(angle)*50.0f
  };

  auto pipeline = gx::Pipeline()
    .alphaBlend()
    .scissor(Ui::scissor_rect(parent.clip(g)))
    .primitiveRestart(0xFFFF)
    ;

  auto alpha = (byte)((sin(angle*2.0f)+1.0f)/2.0f*255.0f);

  vec2 xo = { c.x + sin(angle)*30.0f, c.y };
  float xr = 10.0f;

  Color slider_color = style.button.color[0].luminance();

  painter
    .pipeline(pipeline)
    .rect(g, style.bg.color)
    .border(g, 1, style.border.color)
    .text(*style.font.get(), title, title_pos, white())
    //.border({ g.x+5.0f, title_pos.y-style.font->ascender()-style.font->descener(),
    //        style.font->width(style.font->string(title)),
    //        style.font->height(style.font->string(title)) }, white())
    //.circleSegment(circle, 180.0f, angle, angle + (PI/2.0f), transparent(), white())
    .line(c-vec2{ 50.0f, 0.0f }, c+vec2{ 50.0f, 0.0f }, 10, VertexPainter::CapRound, slider_color, slider_color)
    .lineBorder(c-vec2{ 50.0f, 0.0f }, c+vec2{ 50.0f, 0.0f }, 10-1, VertexPainter::CapRound, black(), black())
    //.rect({ c.x, c.y+m_animation.channel<float>(0), 50.0f, 50.0f }, Color(255, 0, 0))
    .rect({ c.x, c.y+m_animation.channel<float>(1), 50.0f, 50.0f }, m_animation.channel<Color>(0))
    ;
}

}