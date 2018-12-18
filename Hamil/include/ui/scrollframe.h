#pragma once

#include <common.h>

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/frame.h>

namespace ui {

class ScrollFrame : public Frame {
public:
  enum Scrollbars :uint {
    None = 0,

    // Horizontal scrollbar
    HScrollbar = 1<<0,
    // Vertical scrollbar
    VScrollbar = 1<<1,

    AllScrollbars = ~0u,
  };

  using Frame::Frame;
  ~ScrollFrame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  // Takes a bitwise OR of 'Scrollbars' enum
  //   values which signify enabled scrollbars
  //  - Only VScrollbar is enabled by default
  ScrollFrame& scrollbars(uint /* Scrollbars */ sb);

  ScrollFrame& content(Frame *frame);
  ScrollFrame& content(Frame& frame);

  virtual vec2 sizeHint() const;

private:
  static constexpr float ScrollbarSize = 5.0f;
  static constexpr float ScrollbarMargin = 2.0f;
  static constexpr float ScrollbarClickMargin = 10.0f;

  bool hasVScrollbar() const;
  bool hasHScrollbar() const;

  // Returns the region currently covered by the scrollbar
  //   - Call this ONLY when content is present!
  Geometry getVScrollbarGeometry() const;
  // Returns the region currently covered by the scrollbar
  //   - Call this ONLY when content is present!
  Geometry getHScrollbarGeometry() const;

  // Covers all possible locations of the scrollbar
  //   expanded by the ScrollbarClickMargin
  Geometry getVScrollbarRegion() const;
  // Covers all possible locations of the scrollbar
  //   expanded by the ScrollbarClickMargin
  Geometry getHScrollbarRegion() const;

  // Update m_content's position.y
  void scrollContentY(float y);
  // Update m_content's position.x
  void scrollContentX(float x);

  bool scrollMousewheel(const win32::Mouse *mouse);

  // Returns 'true' when the mouse intersected a scrollbar
  //   - Captures input when 'true' is returned!
  bool scrollbarClicked(CursorDriver& cursor, const win32::Mouse *mouse);

  uint m_scrollbars = VScrollbar;
  Frame *m_content = nullptr;
  vec2 m_scroll = vec2::zero();
};

}