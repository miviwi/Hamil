#include "uniforms.h"

namespace U {

program_klass program;
const std::array<Location, 5> program_klass::offsets = {
  Location{ "uModelView", 0 },
  Location{ "uProjection", 1 },
  Location{ "uNormal", 2 },
  Location{ "uCol", 3 },
  Location{ "uLightPosition", 4 },
};


tex_klass tex;
const std::array<Location, 5> tex_klass::offsets = {
  Location{ "uModelView", 0 },
  Location{ "uProjection", 1 },
  Location{ "uNormal", 2 },
  Location{ "uTexMatrix", 3 },
  Location{ "uTex", 4 },
};


font_klass font;
const std::array<Location, 3> font_klass::offsets = {
  Location{ "uModelViewProjection", 0 },
  Location{ "uAtlas", 1 },
  Location{ "uColor", 2 },
};


ui_klass ui;
const std::array<Location, 4> ui_klass::offsets = {
  Location{ "uModelViewProjection", 0 },
  Location{ "uType", 1 },
  Location{ "uFontAtlas", 2 },
  Location{ "uTextColor", 3 },
};


cursor_klass cursor;
const std::array<Location, 2> cursor_klass::offsets = {
  Location{ "uModelViewProjection", 0 },
  Location{ "uTex", 1 },
};

}