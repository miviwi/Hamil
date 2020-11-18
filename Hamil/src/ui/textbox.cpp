#include <ui/textbox.h>
#include <os/clipboard.h>

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
    
  if(auto mouse = input->get<os::Mouse>()) {
    using os::Mouse;

    auto pos = cursor.pos() - g.pos();

    if(mouse_over) {
      switch(mouse->event) {
      case Mouse::Down:
        m_state = Selecting;
        m_cursor = placeCursor(pos);
        m_selection = SelectionRange::begin(m_cursor);

        ui().keyboard(this);
        break;

      case Mouse::Up: m_state = Editing; break;
      case Mouse::DoubleClick: selection(selectWord(pos)); break;

      default: return mouseGesture(pos);
      }

      return true;
    } else {
      // The TextBoxFrame should remain editable after
      //   the mouse leaves it's bounds
      if(m_state == Editing) return true;

      switch(mouse->event) {
      case Mouse::Down:    // Clicked outside bounds - release focus
        m_state = Default;

        ui().keyboard(nullptr);
        return false;
      }
    }

  } else if(auto kb = input->get<os::Keyboard>()) {
    if(m_state != Editing || kb->event != os::Keyboard::KeyDown) return false;

    cursor.visible(!mouse_over);
    return keyboardDown(cursor, kb);
  }

  return false;
}

void TextBoxFrame::paint(VertexPainter& painter, Geometry parent)
{
  const auto& style = ownStyle();
  const auto& textbox = style.textbox;
  auto& fnt = *font();

  Geometry g = geometry(),
    clipped_g = parent.clip(g);

  Color cursor_color = textbox.cursor;
  auto cursor_alpha = m_cursor_blink.channel<float>(0);
  switch(m_state) {
  case Editing: cursor_color = cursor_color.opacity(cursor_alpha); break;
  }

  Color border_color = textbox.border_color[0];
  switch(m_state) {
  case Editing:
  case Selecting:
    border_color = textbox.border_color[1]; break;
  }

  vec2 text_pos = {
    g.x + TextPixelMargin,
    g.y + fnt.bearingY() + TextPixelMargin
  };

  float cursor_x = text_pos.x + cursorX(m_cursor);

  Geometry cursor_g = {
    cursor_x, g.y + 1.0f,
    1.0f, g.h - TextPixelMargin
  };

  auto text_pipeline = painter.defaultPipeline(ui().scissorRect(clipped_g));

  painter
    .pipeline(text_pipeline)
    .rect(textbox.border ? g.contract(1) : g, textbox.bg)
    .rect(cursor_g, cursor_color)
    .border(g.contract(1), 2.0f, textbox.border ? border_color : textbox.bg)
    ;

  if(m_text.empty() && !m_hint.empty()) { // Draw hint
    painter
      .text(fnt, m_hint, text_pos, textbox.text.opacity(0.35))
      ;
  } else {
    painter
      .text(fnt, m_text, text_pos, textbox.text)
      ;
  }

  if(m_selection.valid()) {
    std::pair<float, float> sx = {
      cursorX(m_selection.left()),
      cursorX(m_selection.right()),
    };

    Geometry selection_g = {
      text_pos.x + sx.first, g.y + 1.0f,
      sx.second - sx.first, g.x - TextPixelMargin
    };

    painter
      .rect(selection_g, textbox.selection.opacity(0.5))
      ;
  }
}

void TextBoxFrame::losingCapture()
{
  m_state = Default;
}

void TextBoxFrame::attached(Frame *parent)
{
  Frame::attached(parent);

  m_cursor_blink.start();
}

const ft::Font::Ptr& TextBoxFrame::font() const
{
  return m_font ? m_font : ownStyle().font;
}

TextBoxFrame& TextBoxFrame::font(const ft::Font::Ptr& font)
{
  m_font = font;
  return *this;
}

const std::string& TextBoxFrame::text() const
{
  return m_text;
}

TextBoxFrame& TextBoxFrame::text(const std::string& s)
{
  m_text = s;
  m_cursor = m_text.size();
  m_selection.reset();

  return *this;
}

const std::string& TextBoxFrame::hint() const
{
  return m_hint;
}

TextBoxFrame& TextBoxFrame::hint(const std::string& s)
{
  m_hint = s;

  return *this;
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

TextBoxFrame& TextBoxFrame::onSubmit(OnSubmit::Slot on_submit)
{
  m_on_submit.connect(on_submit);

  return *this;
}

TextBoxFrame::OnSubmit& TextBoxFrame::submit()
{
  return m_on_submit;
}

TextBoxFrame& TextBoxFrame::onKeyDown(OnKeyDown::Slot on_key_down)
{
  m_on_key_down.connect(on_key_down);

  return *this;
}

TextBoxFrame::OnKeyDown& TextBoxFrame::keyDown()
{
  return m_on_key_down;
}

vec2 TextBoxFrame::sizeHint() const
{
  return { 0, font()->height() + TextPixelMargin*2 };
}

bool TextBoxFrame::keyboardDown(CursorDriver& cursor, os::Keyboard *kb)
{
  bool handled = kb->special() ? specialInput(kb) : charInput(kb);

  m_on_key_down.emit(this, kb);
  return handled;
}

bool TextBoxFrame::charInput(os::Keyboard *kb)
{
  using os::Keyboard;

#if 0
  STUB();

  if(iscntrl(kb->key) || kb->modifier(Keyboard::Ctrl)) {
    switch(kb->key) {
    case 'A': // Select all
      m_selection = { 0, m_text.size() };
      m_cursor = m_text.size();
      break;

    case 'X':   // Cut
    case 'C': { // Copy
      win32::Clipboard clipboard;
      auto str = m_text.data() + m_selection.left();

      clipboard.string(str, m_selection.size());
      if(kb->key == 'X') doDeleteSelection();
    }
    break;
    case 'V': { // Paste
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
#endif

  doDeleteSelection();

  m_text.insert(m_cursor, 1, (char)kb->sym);
  m_cursor++;

  m_selection.reset();

  return true;
}

bool TextBoxFrame::specialInput(os::Keyboard *kb)
{
  using os::Keyboard;

  m_cursor_blink.start();

  switch(kb->key) {
  case Keyboard::Right:
    cursor(m_cursor+1);
    if(kb->modifier(Keyboard::Shift)) {
      selection(m_selection.valid() ? m_selection.first : m_cursor-1, m_cursor);
    } else {
      m_selection.reset();
    }
    break;
  case Keyboard::Left:
    cursor(m_cursor-1);
    if(kb->modifier(Keyboard::Shift)) {
      selection(m_selection.valid() ? m_selection.first : m_cursor+1, m_cursor);
    } else {
      m_selection.reset();
    }
    break;
  case Keyboard::Up:
  case Keyboard::Home:
    if(kb->modifier(Keyboard::Shift)) {
      selection(0, m_selection.valid() ? m_selection.right() : m_cursor);
    } else {
      m_selection.reset();
    }
    m_cursor = 0;
    break;
  case Keyboard::Down:
  case Keyboard::End:
    if(kb->modifier(Keyboard::Shift)) {
      selection(m_selection.valid() ? m_selection.left() : m_cursor, m_text.size());
    } else {
      m_selection.reset();
    }
    m_cursor = m_text.size();
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
  }

  return true;
}

float TextBoxFrame::cursorX(size_t index) const
{
  auto& fnt = *font();

  float x = 0.0f;
  for(size_t i = 0; i < index; i++) x += fnt.charAdvance(m_text[i]);

  return x;
}

size_t TextBoxFrame::placeCursor(vec2 pos) const
{
  auto& fnt = *font();

  float x = TextPixelMargin;
  size_t cursor = 0;
  for(; cursor < m_text.size(); cursor++) {
    float advance = fnt.charAdvance(m_text[cursor]);

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

  [[maybe_unused]] auto r = SelectionRange::none();

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

void TextBoxFrame::cursor(size_t x)
{
  m_cursor = clamp(x, (size_t)0, m_text.size());
}

bool TextBoxFrame::doDeleteSelection()
{
  if(!m_selection.valid()) return false;

  size_t off = m_selection.left(),
    count = m_selection.size();

  m_text.erase(off, count);
  cursor(m_selection.left());
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
