#include "uniforms.h"

U__::program__ U__::program;
const std::array<U__::Location, 5> U__::program__::offsets = {
  Location{ "uModelView", 0 },
  Location{ "uProjection", 1 },
  Location{ "uNormal", 2 },
  Location{ "uCol", 3 },
  Location{ "uLightPosition", 4 },
};

U__::tex__ U__::tex;
const std::array<U__::Location, 5> U__::tex__::offsets = {
  Location{ "uModelView", 0 },
  Location{ "uProjection", 1 },
  Location{ "uNormal", 2 },
  Location{ "uTexMatrix", 3 },
  Location{ "uTex", 4 },
};

U__::font__ U__::font;
const std::array<U__::Location, 3> U__::font__::offsets = {
  Location{ "uModelViewProjection", 0 },
  Location{ "uAtlas", 1 },
  Location{ "uColor", 2 },
};

U__::cursor__ U__::cursor;
const std::array<U__::Location, 2> U__::cursor__::offsets = {
  Location{ "uModelViewProjection", 0 },
  Location{ "uTex", 1 },
};

U__::ui__ U__::ui;
const std::array<U__::Location, 6> U__::ui__::offsets = {
  Location{ "uModelViewProjection", 0 },
  Location{ "uType", 1 },
  Location{ "uImagePage", 2 },
  Location{ "uFontAtlas", 3 },
  Location{ "uImageAtlas", 4 },
  Location{ "uTextColor", 5 },
};

U__ U;