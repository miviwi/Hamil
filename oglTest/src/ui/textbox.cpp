#include <ui/textbox.h>
#include <win32/clipboard.h>

#include <cmath>
#include <cctype>

namespace ui {

size_t SelectionRange::left() const
{
  return std::min(first, last);
}

size_t SelectionRange::right() const
{
  return std::max(first, last);
}

size_t SelectionRange::size() const
{
  long long d = first - last;

  return abs(d);
}

TextBoxFrame::~TextBoxFrame()
{
}

bool TextBoxFrame::input(CursorDriver& cursor, const InputPtr& input)
{
  auto g = geometry();
  bool mouse_over = g.intersect(cursor.pos());
    
  if(auto mouse = input->get<win32::Mouse>()) {
    using win32::Mouse;

    auto pos = cursor.pos() - g.pos();

    if(mouse_over) {
      switch(mouse->event) {
      case Mouse::Down:
        m_state = Selecting;
        m_cursor = placeCursor(pos);
        m_selection = SelectionRange::begin(m_cursor);

        m_ui->keyboard(this);
        break;

      case Mouse::Up:
        m_state = Editing;
        break;

      case Mouse::DoubleClick:
        selection(selectWord(pos));
        break;

      default: return mouseGesture(pos);
      }

      return true;
    } else {
      if(mouse->event == Mouse::Down) {
        m_state = Default;

        m_ui->keyboard(nullptr);
        return false;
      }
    }
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

  Color border_color = black();
  switch(m_state) {
  case Editing:
  case Selecting:
    border_color = Color(112, 112, 255); break;
  }

  vec2 text_pos = {
    g.x + TextPixelMargin,
    g.y + font.bearingY() + TextPixelMargin
  };

  float cursor_x = text_pos.x + cursorX(m_cursor);

  Geometry cursor_g = {
    cursor_x, g.y + 1.0f,
    1.0f, g.h - TextPixelMargin
  };

  auto text_pipeline = gx::Pipeline()
    .alphaBlend()
    .scissor(Ui::scissor_rect(parent.clip(g)))
    .primitiveRestart(0xFFFF)
    ;

  painter
    .pipeline(text_pipeline)
    .rect(g, white())
    .rect(cursor_g, cursor_color)
    .border(g, 1.0f, border_color)
    .text(font, m_text, text_pos, black())
    ;

  if(m_selection.valid()) {
    std::pair<float, float> sx = {
      cursorX(m_selection.first),
      cursorX(m_selection.last),
    };

    if(sx.first > sx.second) std::swap(sx.first, sx.second);

    Geometry selection_g = {
      text_pos.x + sx.first, g.y + 1.0f,
      sx.second - sx.first, g.x - TextPixelMargin
    };

    painter
      .rect(selection_g, Color(112, 112, 255, 128))
      ;
  }
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

  if(iscntrl(kb->key) || kb->modifier(Keyboard::Ctrl)) {
    switch(kb->key) {
    case 'A':
      m_selection = { 0, m_text.size() };
      m_cursor = m_text.size();
      break;

    case 'X':
    case 'C': {
      win32::Clipboard clipboard;
      auto str = m_text.c_str() + m_selection.left();

      clipboard.string(str, m_selection.size());
      if(kb->key == 'X') doDeleteSelection();
    }
    break;
    case 'V': {
      win32::Clipboard clipboard;
      auto str = clipboard.string();

      doDeleteSelection();

      m_text.insert(m_cursor, str);
      m_cursor += str.size();
    }
    break;
    }

    return true;
  }

  doDeleteSelection();

  m_text.insert(m_cursor, 1, (char)kb->sym);
  m_cursor++;

  m_selection.reset();

  return true;
}

bool TextBoxFrame::specialInput(win32::Keyboard *kb)
{
  using win32::Keyboard;

  m_cursor_blink.start();

  switch(kb->key) {
  case Keyboard::Right:
    m_cursor = clamp((int)m_cursor+1, 0, (int)m_text.size());
    if(kb->modifier(Keyboard::Shift)) {
      selection(m_selection.valid() ? m_selection.first : m_cursor-1, m_cursor);
    } else {
      m_selection.reset();
    }
    break;
  case Keyboard::Left:
    m_cursor = clamp((int)m_cursor-1, 0, (int)m_text.size());
    if(kb->modifier(Keyboard::Shift)) {
      selection(m_selection.valid() ? m_selection.first : m_cursor+1, m_cursor);
    } else {
      m_selection.reset();
    }
    break;

  case Keyboard::Enter: m_on_submit.emit(this); break;

  case Keyboard::Backspace:
    if(doDeleteSelection()) break;
    if(m_text.empty() || !m_cursor) break;

    m_text.erase(m_cursor-1, 1);
    m_cursor--;
    break;
  case Keyboard::Delete:
    if(doDeleteSelection()) break;
    if(m_text.empty() || m_cursor == m_text.size()) break;

    m_text.erase(m_cursor, 1);
    break;

  case Keyboard::Home:
    if(kb->modifier(Keyboard::Shift)) {
      selection(0, m_selection.valid() ? m_selection.right() : m_cursor);
    } else {
      m_selection.reset();
    }
    m_cursor = 0;
    break;
  case Keyboard::End:
    if(kb->modifier(Keyboard::Shift)) {
      selection(m_selection.valid() ?  m_selection.left() : m_cursor, m_text.size());
    } else {
      m_selection.reset();
    }
    m_cursor = m_text.size();
    break;
  }

  return true;
}

float TextBoxFrame::cursorX(size_t index) const
{
  auto& font = *m_ui->style().font;

  float x = 0.0f;
  for(size_t i = 0; i < index; i++) x += font.charAdvance(m_text[i]);

  return x;
}

size_t TextBoxFrame::placeCursor(vec2 pos) const
{
  auto& font = *m_ui->style().font;

  float x = TextPixelMargin;
  size_t cursor = 0;
  for(; cursor < m_text.size(); cursor++) {
    float advance = font.charAdvance(m_text[cursor]);

    if(x + advance/2.0f > pos.x) break;

    x += advance;
  }

  return cursor;
}

SelectionRange TextBoxFrame::selectWord(vec2 pos) const
{
  size_t cursor = placeCursor(pos);

  if(cursor == m_text.size()) {
    return { 0, m_text.size() };
  }

  auto r = SelectionRange::none();

  auto word_start = std::string::npos,
    word_end = std::string::npos;

  if(m_text[cursor] != ' ') {
    word_start = m_text.find_last_of(' ', cursor);

    word_end = m_text.find_first_of(' ', cursor);       // find the first space
    word_end = m_text.find_first_not_of(' ', word_end); // ...and then the last
  } else {
    word_start = m_text.find_last_not_of(' ', cursor); // find the last non-space character
    if(word_start != std::string::npos) word_start = m_text.find_last_of(' ', word_start); 

    word_end = m_text.find_first_not_of(' ', cursor);
  }

  return {
    word_start != std::string::npos ? word_start+1 : 0,
    word_end != std::string::npos ? word_end : m_text.size()
  };
}

bool TextBoxFrame::doDeleteSelection()
{
  if(!m_selection.valid()) return false;

  size_t off = m_selection.left(),
    count = m_selection.size();

  m_text.erase(off, count);
  m_cursor = (m_cursor > m_selection.first ? m_selection.last : m_selection.first) - count;

  m_selection.reset();

  return true;
}

bool TextBoxFrame::mouseGesture(vec2 pos)
{
  switch(m_state) {
  case Selecting:
    m_cursor = placeCursor(pos);

    selection(m_selection.first, m_cursor);
    break;
  }

  return true;
}

SelectionRange TextBoxFrame::selection() const
{
  return m_selection;
}

void TextBoxFrame::selection(SelectionRange r)
{
  selection(r.first, r.last);
}

void TextBoxFrame::selection(size_t first, size_t last)
{
  m_selection = {
    clamp(first, (size_t)0, m_text.size()),
    clamp(last, (size_t)0, m_text.size())
  };
  
  m_cursor = m_selection.last;
}

}