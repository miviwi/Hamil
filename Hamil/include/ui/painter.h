#pragma once

#include <common.h>

#include <ui/uicommon.h>
#include <math/geometry.h>
#include <gx/program.h>
#include <gx/buffer.h>
#include <gx/vertex.h>
#include <gx/pipeline.h>
#include <ft/font.h>

#include <string>
#include <vector>
#include <initializer_list>

namespace ui {

class Drawable;

struct Vertex {
  enum : unsigned {
    RestartIndex = 0xFFFF,
  };

  Position pos;
  union {
    Color color;
    UV uv;
  };

  Vertex();
  Vertex(vec2 pos_, Color color_);
  Vertex(Position pos_, Color color_);
};

// TODO:
//    - cache painted geometry (with a lot of vertices) based on size and color,
//      when retrieving translate to appropriate place
class VertexPainter {
public:
  enum { BufferSize = 256*1024 };

  enum CommandType {
    Text, Image, Primitive, Pipeline,
  };

  struct Command {
    CommandType type;

    union {
      struct {
        gx::Primitive p;
        size_t base, offset;
        size_t num;

        ft::Font *font;
        vec2 pos; Color color;

        unsigned page;
      };

      gx::Pipeline pipeline;
    };

    Command() : pipeline() { }

    static Command primitive(gx::Primitive prim, size_t base, size_t offset, size_t num)
    {
      Command c;
      c.type = Primitive;
      c.p = prim;
      c.base = base; c.offset = offset;
      c.num = num;

      return c;
    }

    static Command text(ft::Font& font, vec2 pos, Color color, size_t base, size_t offset, size_t num)
    {
      Command c;
      c.type = Text;
      c.p = gx::TriangleFan;
      c.base = base; c.offset = offset;
      c.num = num;
      
      c.font = &font;
      c.pos = pos; c.color = color;

      return c;
    }

    static Command image(vec2 pos, unsigned page, size_t base, size_t offset, size_t num)
    {
      Command c;
      c.type = Image;
      c.p = gx::TriangleFan;
      c.base = base; c.offset = offset;
      c.num = num;

      c.pos = pos;
      c.page = page;

      return c;
    }

    static Command switch_pipeline(const gx::Pipeline& pipeline)
    {
      Command c;
      c.type = Pipeline;
      c.pipeline = pipeline;

      return c;
    }
  };

  enum Corner {
    TopLeft     = (1<<0),
    BottomLeft  = (1<<1),
    BottomRight = (1<<2),
    TopRight    = (1<<3),

    All = 0xF,
  };

  enum LineCap {
    CapRound, CapSquare, CapButt,
  };

  enum LineJoin {
    JoinRound, JoinBevel,
  };

  struct Error { };

  struct NotEnoughSpaceError { };

  static const gx::VertexFormat Fmt;

  VertexPainter();

  VertexPainter& begin(Vertex *verts, size_t num_verts, u16 *inds, size_t num_inds);

  VertexPainter& line(vec2 a, vec2 b, float width, LineCap cap, float cap_r, Color ca, Color cb);
  VertexPainter& line(vec2 a, vec2 b, float width, LineCap cap, Color ca, Color cb);
  VertexPainter& lineBorder(vec2 a, vec2 b, float width, LineCap cap, float cap_r, Color ca, Color cb);
  VertexPainter& lineBorder(vec2 a, vec2 b, float width, LineCap cap, Color ca, Color cb);
  VertexPainter& rect(Geometry g, Color a, Color b, Color c, Color d);
  VertexPainter& rect(Geometry g, const Color c[4]);
  VertexPainter& rect(Geometry g, Color c);
  VertexPainter& border(Geometry g, float width, Color a, Color b, Color c, Color d);
  VertexPainter& border(Geometry g, float width, const Color c[4]);
  VertexPainter& border(Geometry g, float width, Color c);
  VertexPainter& circleSegment(vec2 pos, float radius, float start_angle, float end_angle, Color a, Color b);
  VertexPainter& circle(vec2 pos, float radius, Color a, Color b);
  VertexPainter& circle(vec2 pos, float radius, Color c);
  VertexPainter& arc(vec2 pos, float radius, float start_angle, float end_angle, Color c);
  VertexPainter& arcFull(vec2 pos, float radius, Color c);
  VertexPainter& roundedRect(Geometry g, float radius, unsigned corners, Color a, Color b);
  VertexPainter& roundedRect(Geometry g, float radius, unsigned corners, Color c);
  VertexPainter& roundedBorder(Geometry g, float radius, unsigned corners, Color c);
  VertexPainter& triangle(vec2 pos, float h, float angle, Color c);

  VertexPainter& text(ft::Font& font, const std::string& str, vec2 pos, Color c);
  VertexPainter& textCentered(ft::Font& font, const std::string& str, Geometry g, Color c);
  VertexPainter& textLeft(ft::Font& font, const std::string& str, Geometry g, Color c);

  VertexPainter& drawable(const Drawable& drawable, vec2 pos);
  VertexPainter& drawableCentered(const Drawable& drawable, Geometry g);
  VertexPainter& drawableLeft(const Drawable& drawable, Geometry g);

  VertexPainter& text(const Drawable& text, vec2 pos);
  VertexPainter& textCentered(const Drawable& text, Geometry g);
  VertexPainter& textLeft(const Drawable& text, Geometry g);

  VertexPainter& image(const Drawable& image, vec2 pos);
  VertexPainter& imageCentered(const Drawable& image, Geometry g);
  VertexPainter& imageLeft(const Drawable& image, Geometry g);

  VertexPainter& pipeline(const gx::Pipeline& pipeline);

  VertexPainter& beginOverlay();
  VertexPainter& endOverlay();

  template <typename Fn>
  void doCommands(Fn fn)
  {
    for(auto& command : m_commands) fn(command);
    for(auto& command : m_overlay_commands) fn(command);
  }

  void end();

private:
  size_t currentBase() const;
  size_t currentOffset() const;

  void appendVertices(std::initializer_list<Vertex> verts);
  void appendCommand(const Command& c);
  void doAppendCommand(const Command& c, std::vector<Command>& commands);
  void restartPrimitive();

  ft::String appendTextVertices(ft::Font& font, const std::string& str);

  vec2 textAlignCenter(ft::Font& font, Geometry g, vec2 text_size) const;
  vec2 textAlignLeft(ft::Font& font, Geometry g, vec2 text_size) const;

  void assertBegin();

  void checkEnoughSpace(size_t sz);

  bool m_overlay;

  Vertex *m_buf;
  Vertex *m_buf_rover;
  Vertex *m_buf_end;

  u16 *m_ind;
  u16 *m_ind_rover;
  u16 *m_ind_end;

  std::vector<Command> m_commands;
  std::vector<Command> m_overlay_commands;
};

}
