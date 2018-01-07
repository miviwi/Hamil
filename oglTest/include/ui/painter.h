#pragma once

#include <common.h>

#include "vmath.h"
#include "program.h"
#include "buffer.h"
#include "vertex.h"
#include "pipeline.h"
#include "font.h"
#include "ui/common.h"

#include <vector>

namespace ui {

struct Vertex {
  Position pos;
  Color color;

  Vertex();
  Vertex(vec2 pos_, Color color_);
};

class VertexPainter {
public:
  enum { InitialBufferReserve = 1024 };

  enum CommandType {
    Text, Primitive, Pipeline,
  };

  struct Command {
    CommandType type;

    gx::Primitive p;
    size_t offset, num;

    ft::Font *font;
    ft::String str;
    vec2 pos; Color color;

    gx::Pipeline pipel;

    static Command primitive(gx::Primitive prim, size_t offset, size_t num)
    {
      Command c;

      c.type = Primitive;
      c.p = prim;
      c.offset = offset;  c.num = num;

      return c;
   }

    static Command text(ft::Font *font, ft::String str, vec2 pos, Color color)
    {
      Command c;

      c.type = Text;
      c.font = font;
      c.str = str;
      c.pos = pos; c.color = color;

      return c;
    }

    static Command pipeline(const gx::Pipeline& pipeline)
    {
      Command c;

      c.type = Pipeline;
      c.pipel = pipeline;

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

  static const gx::VertexFormat Fmt;

  VertexPainter();

  VertexPainter& rect(Geometry g, Color a, Color b, Color c, Color d);
  VertexPainter& rect(Geometry g, const Color c[4]);
  VertexPainter& rect(Geometry g, Color c);
  VertexPainter& border(Geometry g, Color a, Color b, Color c, Color d);
  VertexPainter& border(Geometry g, const Color c[4]);
  VertexPainter& border(Geometry g, Color c);
  VertexPainter& circleSegment(vec2 pos, float radius, float start_angle, float end_angle, Color a, Color b);
  VertexPainter& circle(vec2 pos, float radius, Color a, Color b);
  VertexPainter& circle(vec2 pos, float radius, Color c);
  VertexPainter& arc(vec2 pos, float radius, float start_angle, float end_angle, Color c);
  VertexPainter& arcFull(vec2 pos, float radius, Color c);
  VertexPainter& roundedRect(Geometry g, float radius, unsigned corners, Color a, Color b);
  VertexPainter& roundedBorder(Geometry g, float radius, unsigned corners, Color c);

  VertexPainter& text(ft::Font& font, const char *str, vec2 pos, Color c);
  VertexPainter& text(ft::Font& font, ft::String str, vec2 pos, Color c);
  VertexPainter& textCentered(ft::Font& font, ft::String str, Geometry g, Color c);

  VertexPainter& pipeline(const gx::Pipeline& pipeline);

  void uploadVerts(gx::VertexBuffer& buf);

  template <typename Fn>
  void doCommands(Fn fn)
  {
    for(auto& command : m_commands) fn(command);
  }

private:
  std::vector<Vertex> m_buf;

  std::vector<Command> m_commands;
};

}
