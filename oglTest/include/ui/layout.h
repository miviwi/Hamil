#pragma once

#include <ui/uicommon.h>
#include <ui/ui.h>
#include <ui/frame.h>

#include <vector>
#include <memory>
#include <utility>

namespace ui {

class LayoutFrame : public Frame {
public:
  using Frame::Frame;
  virtual ~LayoutFrame();

  virtual bool input(CursorDriver& cursor, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  LayoutFrame& frame(Frame *frame);
  LayoutFrame& frame(Frame& frame);
  template <typename T, typename... Args>
  LayoutFrame& frame(Args&&... args)
  {
    Frame *f = new T(std::forward<Args>(args)...);
    return frame(f);
  }

  Frame& getFrameByIndex(unsigned idx);

protected:
  virtual void calculateFrameGeometries() = 0;

  std::vector<Frame *> m_frames;
};

// Frame Geometry:
//   - position is ignored
//   - height is mandatory
//   - width, if not given, is set to the layout's width,
//     if given the frame is placed according to it's Gravity
class RowLayoutFrame : public LayoutFrame {
public:
  using LayoutFrame::LayoutFrame;
  virtual ~RowLayoutFrame();

  vec2 sizeHint() const;

protected:
  virtual void calculateFrameGeometries();
};

// Frame Geometry:
//   - position is ignored
//   - width is mandatory
//   - height, if not given, is set to the layout's height,
//     if given the frame is placed according to it's Gravity
class ColumnLayoutFrame : public LayoutFrame {
public:
  using LayoutFrame::LayoutFrame;
  virtual ~ColumnLayoutFrame();

  vec2 sizeHint() const;

protected:
  virtual void calculateFrameGeometries();
};

}