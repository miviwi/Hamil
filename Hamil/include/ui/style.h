#pragma once

#include <ui/uicommon.h>
#include <ui/painter.h>
#include <ft/font.h>

#include <cstring>

namespace gx {
class ResourcePool;
}

namespace ui {

class Ui;

struct Style {
  using Corner = VertexPainter::Corner;

  struct {
    struct {
      ft::FontFamily family = "";
      unsigned height;
    } font;

    struct {
      ft::FontFamily family = "";
      unsigned height;
    } monospace;
  } font_style;

  ft::Font::Ptr font, monospace;

  struct {
    Color color[4];
  } bg;

  struct {
    float radius;
    vec2 margin;
    Color bg;
  } window;

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

  struct {
    Color bg;
    Color text;
    Color selection;
    bool border;
    Color border_color[2];
    Color cursor;
  } textbox;

  Style() { memset(rawPtr(), 0, sizeof(*this)); }

  static Style basic_style();

protected:
  void init(gx::ResourcePool& pool);

private:
  friend Ui;

  void *rawPtr() { return (void *)this; }
};



}
