#pragma once

#include <common.h>

#include "vmath.h"
#include "input.h"
#include "window.h"
#include "ui/common.h"
#include "ui/style.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>

namespace ui {

void init();
void finalize();

class VertexPainter;
class Frame;

using InputPtr = win32::Input::Ptr;

class Ui {
public:
  static const vec2 FramebufferSize;

  Ui(Geometry geom, const Style& style);
  Ui(const Ui& other) = delete;
  ~Ui();

  Ui& operator=(const Ui& other) = delete;

  static ivec4 scissor_rect(Geometry g);

  Ui& frame(Frame *frame);
  template <typename T, typename... Args>
  Ui& frame(Args&&... args)
  {
    Frame *f = new T(std::forward<Args>(args)...);
    return frame(f);
  }

  void registerFrame(Frame *frame);
  Frame *getFrameByName(const std::string& name);
  template <typename T>
  T *getFrameByName(const std::string& name)
  {
    return (T *)getFrameByName(name);
  }

  const Style& style() const;

  bool input(ivec2 mouse_pos, const InputPtr& input);
  void paint();

private:
  Geometry m_geom;
  Style m_style;
  std::vector<Frame *> m_frames;
  std::unordered_map<std::string, Frame *> m_names;
};

}