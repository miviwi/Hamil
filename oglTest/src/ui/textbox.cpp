#include <ui/textbox.h>

#include <cctype>

namespace ui {

TextBoxFrame::~TextBoxFrame()
{
}

bool TextBoxFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  auto g = geometry();
  bool mouse_over = g.intersect(cursor.pos());
    
  if(auto mouse = input->get<win32::Mouse>()) {
    using win32::Mouse;

    if(mouse_over) {
      switch(mouse->event) {
      case Mouse::Down:
        m_state = Selecting;
        m_cursor = placeCursor(cursor.pos()-g.pos());

        m_ui->keyboard(this);
        break;

      case Mouse::Up:
        m_state = Editing;
        break;

      case Mouse::DoubleClick:
        m_text += "a";
        break;
      }
    } else {
      if(mouse->event == Mouse::Down) {
        m_state = Default;

        m_ui->keyboard(nullptr);
        return false;
      }
    }

    return true;
  } else if(auto kb = input->get<win32::Keyboard>()) {
    if(m_state != Editing || kb->event != win32::Keyboard::KeyDown) return false;

    cursor.visible(!mouse_over);
    return keyboardDown(cursor, kb);
  }

  return false;
}

void TextBoxFrame::paint(VertexPainter& painter, Geometry parent)
{
  Geometry g = geometry();

  const auto& style = m_ui->style();
  auto& font = *style.font;

  Color cursor_color = transparent();
  switch(m_state) {
  case Editing: cursor_color = m_cursor_blink.channel<Color>(0); break;
  }

  vec2 text_pos = {
    g.x + TextPixelMargin,
    g.y + font.bearingY() + TextPixelMargin
  };

  float cursor_x = text_pos.x;
  for(size_t i = 0; i < m_cursor; i++) cursor_x += font.charAdvance(m_text[i]);

  Geometry cursor_g = {
    cursor_x, g.y + 1.0f,
    1.0f, g.h - TextPixelMargin
  };

  auto text_pipeline = gx::Pipeline()
    .alphaBlend()
    .scissor(Ui::scissor_rect(parent.clip(g)))
    .primitiveRestart(0xFFFF)
    ;

  auto cursor_pipeline = gx::Pipeline(text_pipeline)
    ;

  painter
    .pipeline(cursor_pipeline)
    .rect(g, m_state == Selecting ? Color(112, 112, 255) : white())
    .rect(cursor_g, cursor_color)
    .border(g, 1.0f, black())
    .text(font, m_text, text_pos, black())
    ;
}

void TextBoxFrame::losingCapture()
{
  m_state = Default;
}

void TextBoxFrame::attached()
{
  m_cursor_blink.start();
}

const std::string& TextBoxFrame::text() const
{
  return m_text;
}

void TextBoxFrame::text(const std::string& s)
{
  m_text = s;
}

TextBoxFrame& TextBoxFrame::onChange(OnChange::Slot on_change)
{
  m_on_change.connect(on_change);

  return *this;
}

TextBoxFrame::OnChange& TextBoxFrame::change()
{
  return m_on_change;
}

TextBoxFrame& TextBoxFrame::onSubmit(OnChange::Slot on_submit)
{
  m_on_submit.connect(on_submit);

  return *this;
}

TextBoxFrame::OnSubmit& TextBoxFrame::submit()
{
  return m_on_submit;
}

vec2 TextBoxFrame::sizeHint() const
{
  const auto& font = *m_ui->style().font;

  return vec2{ 0, font.height() + TextPixelMargin };
}

bool TextBoxFrame::keyboardDown(CursorDriver& cursor, win32::Keyboard *kb)
{
  return kb->special() ? specialInput(kb) : charInput(kb);
}

bool TextBoxFrame::charInput(win32::Keyboard *kb)
{
  using win32::Keyboard;

  if(iscntrl(kb->key) || kb->modifier(Keyboard::Ctrl)) return true;

  m_text.insert(m_cursor, 1, (char)kb->sym);
  m_cursor++;

  return true;
}

bool TextBoxFrame::specialInput(win32::Keyboard *kb)
{
  using win32::Keyboard;

  switch(kb->key) {
  case Keyboard::Right: m_cursor = clamp((int)m_cursor+1, 0, (int)m_text.size()); break;
  case Keyboard::Left:  m_cursor = clamp((int)m_cursor-1, 0, (int)m_text.size()); break;

  case Keyboard::Enter: m_on_submit.emit(this); break;
  case Keyboard::Backspace: 
    if(m_text.empty() || !m_cursor) break;

    m_text.erase(m_cursor-1, 1);
    m_cursor--;
    break;
  }

  m_cursor_blink.start();

  return true;
}

size_t TextBoxFrame::placeCursor(vec2 pos) const
{
  const auto& style = m_ui->style();
  auto& font = *style.font;

  float x = TextPixelMargin;
  size_t cursor = 0;
  for(; cursor < m_text.size(); cursor++) {
    float advance = font.charAdvance(m_text[cursor]);

    if(x + advance/2.0f > pos.x) break;

    x += advance;
  }

  return cursor;
}

}