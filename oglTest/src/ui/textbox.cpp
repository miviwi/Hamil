#include <ui/textbox.h>

namespace ui {

TextBoxFrame::~TextBoxFrame()
{
}

bool TextBoxFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  return geometry().intersect(cursor.pos());
}

void TextBoxFrame::paint(VertexPainter& painter, Geometry parent)
{
  Geometry g = geometry();

  painter
    .rect(parent.clip(g), white())
    ;
}

}