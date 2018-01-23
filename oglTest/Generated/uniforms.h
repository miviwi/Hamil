#include <array>
#include <string>
#include <utility>

namespace U {

using Location = std::pair<std::string, unsigned>;

struct cursor_klass {
  union { struct {
    int uModelView;
    int uProjection;
    int uTexMatrix;
    int uTex;
    };

    int locations[4];
  };

  static const std::array<Location, 4> offsets;
};
extern cursor_klass cursor;

struct font_klass {
  union { struct {
    int uModelViewProjection;
    int uAtlas;
    int uColor;
    };

    int locations[3];
  };

  static const std::array<Location, 3> offsets;
};
extern font_klass font;

struct program_klass {
  union { struct {
    int uModelView;
    int uProjection;
    int uCol;
    };

    int locations[3];
  };

  static const std::array<Location, 3> offsets;
};
extern program_klass program;

struct ui_klass {
  union { struct {
    int uModelViewProjection;
    int uType;
    int uFontAtlas;
    int uTextColor;
    };

    int locations[4];
  };

  static const std::array<Location, 4> offsets;
};
extern ui_klass ui;

}