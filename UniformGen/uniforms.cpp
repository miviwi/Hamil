#include "uniforms.h"

namespace U {

program_klass program;
const std::unordered_map<std::string, unsigned> program_klass::offsets = {
  { "uModelView", 0 },
  { "uProjection", 1 },
  { "uCol", 2 },
};


cursor_klass cursor;
const std::unordered_map<std::string, unsigned> cursor_klass::offsets = {
  { "uModelView", 0 },
  { "uProjection", 1 },
  { "uTex", 2 },
};


font_klass font;
const std::unordered_map<std::string, unsigned> font_klass::offsets = {
  { "uModelViewProjection", 0 },
  { "uAtlas", 1 },
  { "uColor", 2 },
};

}