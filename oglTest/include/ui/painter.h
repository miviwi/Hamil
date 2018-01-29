#pragma once

#include <common.h>

#include "ui/uicommon.h"
#include "vmath.h"
#include "program.h"
#include "buffer.h"
#include "vertex.h"
#include "pipeline.h"
#include "font.h"

#include <string>
#include <vector>
#include <initializer_list>

namespace ui {

struct Vertex {
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
  enum { NumBufferElements = 256*1024 };

  enum CommandType {
    Text, Primitive, Pipeline,
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

  static const gx::VertexFormat Fmt;

  VertexPainter();

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

  VertexPainter& pipeline(const gx::Pipeline& pipeline);

  VertexPainter& beginOverlay();
  VertexPainter& endOverlay();

  Vertex *vertices();
  size_t numVertices();

  u16 *indices();
  size_t numIndices();

  template <typename Fn>
  void doCommands(Fn fn)
  {
    for(auto& command : m_commands) fn(command);
    for(auto& command : m_overlay_commands) fn(command);
  }

  void end();

private:
  void appendVertices(std::initializer_list<Vertex> verts);
  void appendCommand(const Command& c);
  void doAppendCommand(const Command& c, std::vector<Command>& commands);
  void restartPrimitive();

  ft::String appendTextVertices(ft::Font& font, const std::string& str);

  bool m_overlay;

  std::vector<Vertex> m_buf;
  std::vector<u16> m_ind;

  std::vector<Command> m_commands;
  std::vector<Command> m_overlay_commands;
};

}
