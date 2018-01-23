#include "uniforms.h"

namespace U {

cursor_klass cursor;
const std::array<Location, 4> cursor_klass::offsets = {
  Location{ "uModelView", 0 },
  Location{ "uProjection", 1 },
  Location{ "uTexMatrix", 2 },
  Location{ "uTex", 3 },
};


font_klass font;
const std::array<Location, 3> font_klass::offsets = {
  Location{ "uModelViewProjection", 0 },
  Location{ "uAtlas", 1 },
  Location{ "uColor", 2 },
};


program_klass program;
const std::array<Location, 3> program_klass::offsets = {
  Location{ "uModelView", 0 },
  Location{ "uProjection", 1 },
  Location{ "uCol", 2 },
};


ui_klass ui;
const std::array<Location, 4> ui_klass::offsets = {
  Location{ "uModelViewProjection", 0 },
  Location{ "uType", 1 },
  Location{ "uFontAtlas", 2 },
  Location{ "uTextColor", 3 },
};

}