#pragma once

#include <common.h>

#include "vmath.h"
#include "ui/common.h"

#include <vector>

namespace ui {

void init();
void finalize();

class Frame;

class Ui {
public:
  static const vec2 FramebufferSize;

  Ui(Geometry geom);

  void frame(Frame *frame);
  void paint();

private:
  Geometry m_geom;

  Frame *m_frame;
};

class Frame {
public:
  enum PositionMode {
    Local, Global,
  };

  Frame(Geometry geom);

  Frame& color(Color a, Color b, Color c, Color d);
  Frame& border(float width, Color a, Color b, Color c, Color d);

private:
  friend class Ui;

  void paint(Geometry parent);

  PositionMode m_pos_mode;
  Geometry m_geom;
  Color m_color[4];
  float m_border_width;
  Color m_border_color[4];
};

}