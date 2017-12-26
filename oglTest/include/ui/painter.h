#pragma once

#include <common.h>

#include "vmath.h"
#include "program.h"
#include "buffer.h"
#include "vertex.h"
#include "ui/common.h"

#include <vector>

namespace ui {

struct Vertex {
  vec2 pos;
  Color color;
};

class VertexPainter {
public:
  struct Command {
    gx::Primitive primitive;
    size_t offset, num;
  };

  static const gx::VertexFormat Fmt;

  VertexPainter& rect(Geometry g, Color a, Color b, Color c, Color d);
  VertexPainter& rect(Geometry g, Color c[]);
  VertexPainter& border(Geometry g, Color a, Color b, Color c, Color d);
  VertexPainter& border(Geometry g, Color c[]);
  VertexPainter& circleSegment(vec2 pos, float radius, float start_angle, float end_angle, Color a, Color b);
  VertexPainter& circle(vec2 pos, float radius, Color a, Color b);

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
