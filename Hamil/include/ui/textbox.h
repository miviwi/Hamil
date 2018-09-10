#pragma once

#include <common.h>

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/frame.h>
#include <ui/animation.h>

#include <string>

namespace ui {

struct SelectionRange {
  enum : size_t {
    None = ~0u,
  };

  bool valid() const { return first != None && last != None; }
  void reset() { first = last = None; }

  size_t left() const;
  size_t right() const;

  size_t size() const;

  static SelectionRange none() { return { None, None }; }
  static SelectionRange begin(size_t where) { return { where, None }; }

  size_t first, last;
};

class TextBoxFrame : public Frame {
public:
  using OnChange = Signal<TextBoxFrame *>;
  using OnSubmit = Signal<TextBoxFrame *>;

  using OnKeyDown = Signal<TextBoxFrame *, const InputPtr&>;

  using Frame::Frame;
  virtual ~TextBoxFrame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  virtual void losingCapture();
  virtual void attached();

  const ft::Font::Ptr& font() const;
  TextBoxFrame& font(const ft::Font::Ptr& font);

  const std::string& text() const;
  TextBoxFrame& text(const std::string& s);

  const std::string& hint() const;
  TextBoxFrame& hint(const std::string& s);

  SelectionRange selection() const;
  void selection(SelectionRange r);
  void selection(size_t first, size_t last);

  TextBoxFrame& onChange(OnChange::Slot on_change);
  OnChange& change();
  TextBoxFrame& onSubmit(OnSubmit::Slot on_submit);
  OnSubmit& submit();

  TextBoxFrame& onKeyDown(OnKeyDown::Slot on_key_down);
  OnKeyDown& keyDown();

  virtual vec2 sizeHint() const;

private:
  static constexpr float TextPixelMargin = 3.0f;

  enum State {
    Default, Hover, Editing, Selecting,
  };

  Animation m_cursor_blink = {
    make_animation_channel({
       keyframe(black(),       0.5f),
       keyframe(transparent(), 0.5f),
    }, EaseNone, RepeatLoop)
  };

  bool keyboardDown(CursorDriver& cursor, win32::Keyboard *kb);
  bool charInput(win32::Keyboard *kb);
  bool specialInput(win32::Keyboard *kb);

  // 'pos' must be relative to self
  bool mouseGesture(vec2 pos);

  // 'pos' must be relative to self in all methods below!
  float cursorX(size_t index) const;
  size_t placeCursor(vec2 pos) const;

  SelectionRange selectWord(vec2 pos) const;

  void cursor(size_t x);

  // returns 'true' when a selection was present
  bool doDeleteSelection();

  // this member is not to be used directly, instead
  //   use the font() method when you need access
  ft::Font::Ptr m_font = nullptr;

  State m_state = Default;

  size_t m_cursor = 0;
  std::string m_text, m_hint;
  SelectionRange m_selection = SelectionRange::none();

  OnChange m_on_change;
  OnSubmit m_on_submit;

  OnKeyDown m_on_key_down;
};

}