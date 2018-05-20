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

  static SelectionRange none() { return { None, None }; }

  size_t first, last;
};

class TextBoxFrame : public Frame {
public:
  using OnChange = Signal<TextBoxFrame *>;
  using OnSubmit = Signal<TextBoxFrame *>;

  using Frame::Frame;
  virtual ~TextBoxFrame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  virtual void losingCapture();
  virtual void attached();

  const std::string& text() const;
  void text(const std::string& s);

  SelectionRange selection() const;
  void selection(SelectionRange r);
  void selection(size_t first, size_t last);

  TextBoxFrame& onChange(OnChange::Slot on_change);
  OnChange& change();
  TextBoxFrame& onSubmit(OnChange::Slot on_submit);
  OnSubmit& submit();

  virtual vec2 sizeHint() const;

private:
  static constexpr float TextPixelMargin = 3.0f;

  enum State {
    Default, Hover, Editing, Selecting,
  };

  Animation m_cursor_blink = {
    {
      make_animation_channel({
         keyframe(black(),       0.5f),
         keyframe(transparent(), 0.5f),
      }, EaseNone, RepeatLoop)
    }
  };

  bool keyboardDown(CursorDriver& cursor, win32::Keyboard *kb);
  bool charInput(win32::Keyboard *kb);
  bool specialInput(win32::Keyboard *kb);

  // pos is relative
  float cursorX(size_t index) const;
  size_t placeCursor(vec2 pos) const;

  SelectionRange selectWord(vec2 pos) const;

  State m_state = Default;

  size_t m_cursor = 0;
  std::string m_text;
  SelectionRange m_selection = SelectionRange::none();

  OnChange m_on_change;
  OnSubmit m_on_submit;
};

}