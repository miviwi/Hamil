#pragma once

#include "ui/uicommon.h"
#include "ui/painter.h"
#include "font.h"

#include <cstring>

namespace ui {

struct Style {
  using Corner = VertexPainter::Corner;

  ft::Font::Ptr font;

  struct {
    Color color[4];
  } bg;

  struct {
    float width;
    Color color[4];
  } border;

  struct {
    Color color[2];
    float radius;
    float margin;
  } button;

  struct {
    Color color[2];
    float width;
  } slider;

  struct {
    Color color[2];
    float radius;
  } combobox;

  Style() { memset(this, 0, sizeof(*this)); }
};



}