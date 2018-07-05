#pragma once

#include <common.h>

#include <ui/uicommon.h>
#include <ui/cursor.h>
#include <ui/style.h>
#include <math/geometry.h>
#include <win32/input.h>
#include <win32/window.h>
#include <gx/buffer.h>
#include <gx/vertex.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <utility>

namespace ui {

// must be called AFTER gx::init()!
void init();
void finalize();

class VertexPainter;
class Frame;

using InputPtr = win32::Input::Ptr;

class Ui {
public:
  Ui(Geometry geom, const Style& style);
  Ui(const Ui& other) = delete;
  ~Ui();

  Ui& operator=(const Ui& other) = delete;

  static ivec4 scissor_rect(Geometry g);

  Ui& frame(Frame *frame, vec2 pos);
  Ui& frame(Frame *frame);
  Ui& frame(Frame& frame, vec2 pos);
  Ui& frame(Frame& frame);
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

  bool input(CursorDriver& cursor, const InputPtr& input);
  void paint();

  void capture(Frame *frame);
  void keyboard(Frame *frame);

private:
  Geometry m_geom;
  Style m_style;
  std::vector<Frame *> m_frames;
  std::unordered_map<std::string, Frame *> m_names;

  Frame *m_capture;
  Frame *m_keyboard;

  VertexPainter m_painter;
  bool m_repaint;

  gx::VertexBuffer m_buf;
  gx::IndexBuffer m_ind;

  gx::IndexedVertexArray m_vtx;
};

}