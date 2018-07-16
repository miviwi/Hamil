#include <ui/style.h>

namespace ui {

// TODO:
//   This is temporary code moved over from winmain.cpp
Style Style::basic_style()
{
  Style s;
  s.font = ft::Font::Ptr(new ft::Font(ft::FontFamily("segoeui"), 12));
  s.monospace = ft::Font::Ptr(new ft::Font(ft::FontFamily("consola"), 12));

  auto alpha = (byte)(255*0.95);

  auto color_a = ui::Color{ 150, 150, 45, alpha },
    color_b = ui::Color{ 66, 66, 20, alpha };

  s.bg.color[0] = s.bg.color[3] = color_b;
  s.bg.color[1] = s.bg.color[2] = color_a;

  s.border.color[0] = s.border.color[3] = ui::white();
  s.border.color[1] = s.border.color[2] = ui::transparent();

  s.button.color[0] = color_b; s.button.color[1] = color_b.lighten(10);
  s.button.radius = 3.0f;
  s.button.margin = 1;

  s.slider.color[0] = color_b; s.slider.color[1] = color_b.lighten(10);
  s.slider.width = 10.0f;

  s.combobox.color[0] = color_b; s.combobox.color[1] = color_b.lighten(10);
  s.combobox.radius = 3.0f;

  return s;
}

}