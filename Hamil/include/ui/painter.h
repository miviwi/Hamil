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
    // The shapes are created with TriangleFans
    //   broken up by this index
    //  - See VertexPainter::pipeline()
    RestartIndex = 0xFFFF,
  };

  // Expressed in virtual viewport (1280x720 bottom-left origin)
  //   fixed point 12.4 coordinates (see 'vs_src' in ui.cpp and
  //   Position(vec2) in uicommon.cpp)
  Position pos;
  union {
    // Used for primitives
    Color color;
    // Used for Text and Images
    UV uv;
  };

  Vertex();
  // Converts 'pos_' into the required fixed point format
  Vertex(vec2 pos_, Color color_);
  Vertex(Position pos_, Color color_);
};

struct CommandData_ {
  gx::Primitive p;
  size_t base, offset;
  size_t num;

  // Only used when type == Text
  ft::Font *font;
  vec2 pos; Color color;

  // Page in the Drawable image atlas
  unsigned page;
};

// TODO:
//    - Cache painted geometry (with a lot of vertices) based on size and color,
//      when retrieving translate to appropriate place
//      The Drawable system could be used to easily achieve this, though a major
//      refactoring will be required...
class VertexPainter {
public:
  enum { BufferSize = 256*1024 };

  enum CommandType {
    Text, Image, Primitive, Pipeline,
  };

  // Commands accessed from outside of VertexPainter should be treated
  //  as read-only
  //   - Type Text/Image/Primitive commands should be dispatched as 
  //           drawBaseVertex(p, <VertexArray>, num, base, offset)
  //     calls with the U.ui.uType uniform set to
  //           TypeText, TypeImage, TypeShape
  //     respectively and the U.ui.uModelViewProjection matrix set
  //     to a translation by 'pos' (except when type == Primitive)
  //     multiplied by an xform::ortho() projection
  struct Command {
    // See above
    CommandType type;

    union {
      CommandData_ d;

      // Only valid when type == Pipeline
      gx::Pipeline pipeline;
    };

    Command() : pipeline() { }

    // To be used with Shader_TypeShape
    static Command primitive(gx::Primitive prim, size_t base, size_t offset, size_t num)
    {
      Command c;
      c.type = Primitive;
      c.d.p = prim;
      c.d.base = base; c.d.offset = offset;
      c.d.num = num;

      return c;
    }

    // To be used with Shader_TypeText
    static Command text(ft::Font& font, vec2 pos, Color color, size_t base, size_t offset, size_t num)
    {
      Command c;
      c.type = Text;
      c.d.p = gx::Primitive::TriangleFan;
      c.d.base = base; c.d.offset = offset;
      c.d.num = num;
      
      c.d.font = &font;
      c.d.pos = pos; c.d.color = color;

      return c;
    }

    // To be used with Shader_TypeImage
    static Command image(vec2 pos, unsigned page, size_t base, size_t offset, size_t num)
    {
      Command c;
      c.type = Image;
      c.d.p = gx::Primitive::TriangleFan;
      c.d.base = base; c.d.offset = offset;
      c.d.num = num;

      c.d.pos = pos;
      c.d.page = page;

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

  // Not implemented
  enum LineJoin {
    JoinRound, JoinBevel,
  };

  struct Error { };

  // Thrown when there is not enough space in the Vertex/IndexBuffer
  //   - This error should not be handled in a try-catch
  //     and should be fixed by increasing BufferSize
  //     which controls how much memory is allocated
  struct NotEnoughSpaceError { };

  // Format of vertices emmited by the VertexPainter
  //   - Corresponds to the 'Vertex' class
  static const gx::VertexFormat Fmt;

  VertexPainter();

  static gx::Pipeline defaultPipeline(ivec4 scissor);

  // Must be called before painting anything after calling end()
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

  // Use the specified gx::Pipeline for the following draws
  //   - The passed Pipeline MUST have had
  //        restartPrimitve(Vertex::RestartPrimitive)
  //      called on it!
  VertexPainter& pipeline(const gx::Pipeline& pipeline);

  // Starts painting the overlay - which is a set
  //   of Vertices that get drawn on top of all others
  VertexPainter& beginOverlay();
  // Stops painting the overlay
  //   - beginOverlay() can be used to resume it
  //     but will NOT cause additional overlay layers
  //     to be created
  VertexPainter& endOverlay();

  // Calls 'fn' for each Command emmited during painting
  //   - The commands are issiued in the order of the
  //     respective method calls, except for all methods
  //     called between a begin/endOverlay() pair which
  //     are ordered with respect to each other but after
  //     all other commands
  template <typename Fn>
  void doCommands(Fn fn)
  {
    for(auto& command : m_commands) fn(command);
    for(auto& command : m_overlay_commands) fn(command);
  }

  // Must be called to discard stale state after previous paint
  void end();

private:
  // Must be queried before emmiting vertices and used
  //   as the 'base' parameter for the appropriate
  //   Command constructor
  size_t currentBase() const;
  // Must be queried before emmiting vertices and used
  //   as the 'offset' parameter for the appropriate
  //   Command constructor
  size_t currentOffset() const;

  void appendVertices(std::initializer_list<Vertex> verts);
  void appendVerticesAndIndices(const Drawable& d);
  void appendCommand(const Command& c);
  // Method ONLY for interal use by appendCommand()
  void doAppendCommand(const Command& c, std::vector<Command>& commands);
  // Emit a Vertex::RestartIndex into the IndexBuffer
  //   - Called automatically by appendCommand when needed
  void restartPrimitive();

  ft::String appendTextVertices(ft::Font& font, const std::string& str);

  vec2 textAlignCenter(ft::Font& font, Geometry g, vec2 text_size) const;
  vec2 textAlignLeft(ft::Font& font, Geometry g, vec2 text_size) const;

  void assertBegin();

  // Called by appendVertices() to prevent ovevflowing the Vertex/IndexBuffer
  void checkEnoughSpace(size_t sz);

  // Are we painting the overlay?
  bool m_overlay;

  // VertexBuffer view pointer
  Vertex *m_buf;
  // Current write offset in VertexBuffer
  Vertex *m_buf_rover;
  // End of the VertexBuffer
  //   VertexBuffer size == m_buf_end-m_buf
  Vertex *m_buf_end;

  // IndexBuffer view pointer
  u16 *m_ind;
  // Current write offset in IndexBuffer
  u16 *m_ind_rover;
  // End of the IndexBuffer
  //   IndexBuffer size == m_ind_end-m_ind
  u16 *m_ind_end;

  std::vector<Command> m_commands;
  std::vector<Command> m_overlay_commands;
};

}
