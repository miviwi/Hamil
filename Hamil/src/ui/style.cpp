#include <ui/style.h>

namespace ui {

// TODO:
//   This is temporary code moved over from winmain.cpp
Style Style::basic_style()
{
  Style s;
  s.font_style.font = { ft::FontFamily("segoeui"), 12 };
  s.font_style.monospace = { ft::FontFamily("consola"), 12 };

  auto alpha = (byte)(255*0.95);

  auto color_a = Color{ 150, 150, 45, alpha }.darken(40),
    color_b = Color{ 66, 66, 20, alpha }.darken(40);

  s.bg.color[0] = s.bg.color[3] = color_b;
  s.bg.color[1] = s.bg.color[2] = color_a;

  s.window.radius = 3.0f;
  s.window.margin = { 5.0f, 2.0f };

  s.border.color[0] = s.border.color[3] = ui::white();
  s.border.color[1] = s.border.color[2] = ui::transparent();

  s.button.color[0] = color_b; s.button.color[1] = color_b.lighten(10);
  s.button.radius = 3.0f;
  s.button.margin = 1;

  s.slider.color[0] = color_b; s.slider.color[1] = color_b.lighten(10);
  s.slider.width = 10.0f;

  s.combobox.color[0] = color_b; s.combobox.color[1] = color_b.lighten(10);
  s.combobox.radius = 3.0f;

  s.textbox.bg = black();
  s.textbox.text = white();
  s.textbox.selection = Color(112, 112, 255).darken(40);
  s.textbox.border = false;
  s.textbox.border_color[0] = black();
  s.textbox.border_color[1] = Color(6, 70, 173).darken(40);
  s.textbox.cursor = white();

  return s;
}

void Style::init(gx::ResourcePool& pool)
{
  font.reset(new ft::Font(font_style.font.family, font_style.font.height, &pool));
  monospace.reset(new ft::Font(font_style.monospace.family, font_style.monospace.height, &pool));
}

}