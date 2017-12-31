#pragma once

#include "ui/common.h"
#include "font.h"

#include <cstring>

namespace ui {

struct Style {
  ft::Font::Ptr font;

  struct {
    Color color[4];
    unsigned corners;
  } bg;

  struct {
    float width;
    Color color[4];
  } border;

  Style() { memset(this, 0, sizeof(*this)); }
};



}