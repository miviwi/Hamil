#pragma once

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/frame.h>
#include <math/geometry.h>

#include <string>
#include <vector>

namespace ui {

struct DropDownItem {
  using Id = unsigned;
  enum {
    Invalid = ~0u,
  };

  DropDownItem() :
    id(Invalid)
  { }
  DropDownItem(const std::string& c) :
    id(Invalid), caption(c)
  { }

  Id id;
  std::string caption;
};

class DropDownFrame : public Frame {
public:
  using OnChange = Signal<DropDownFrame *>;

  using Frame::Frame;
  virtual ~DropDownFrame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  DropDownFrame& item(DropDownItem item);

  DropDownFrame& selected(DropDownItem::Id id);
  DropDownItem::Id selected() const;

  DropDownFrame& onChange(OnChange::Slot on_change);
  OnChange& change();

  virtual void losingCapture();

  virtual vec2 sizeHint() const;

private:
  enum State {
    Default, Hover, Pressed,
  };

  static constexpr float ButtonWidth = 20.0f;

  Geometry buttonGeometry() const;
  Geometry itemGeometry(unsigned idx) const;

  bool inputDropped(vec2 mouse_pos, win32::Mouse *mouse);

  State m_state = Default;
  bool m_dropped = false;

  std::vector<DropDownItem> m_items;
  DropDownItem *m_selected = nullptr;
  DropDownItem::Id m_highlighted = DropDownItem::Invalid;

  OnChange m_on_change;
};

}