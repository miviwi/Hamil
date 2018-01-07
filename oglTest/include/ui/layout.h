#pragma once

#include "ui/common.h"
#include "ui/ui.h"
#include "ui/frame.h"

#include <vector>
#include <memory>
#include <utility>

namespace ui {

class StackLayoutFrame : public Frame {
public:
  using Frame::Frame;
  virtual ~StackLayoutFrame();

  virtual bool input(ivec2 mouse_pos, const InputPtr& input);
  virtual void paint(VertexPainter& painter, Geometry parent);

  StackLayoutFrame& frame(Frame *frame);
  template <typename T, typename... Args>
  StackLayoutFrame& frame(Args&&... args)
  {
    Frame *f = new T(std::forward<Args>(args)...);
    return frame(f);
  }

  Frame& getFrameByIndex(unsigned idx);

private:
  friend class Ui;

  void calculateFrameGeometries();

  std::vector<Frame *> m_frames;
};

}