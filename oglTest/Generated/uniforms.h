#include <array>
#include <string>
#include <utility>

namespace U {

using Location = std::pair<std::string, unsigned>;

struct program_klass {
  union { struct {
    int uModelView;
    int uProjection;
    int uNormal;
    int uCol;
    int uLightPosition;
    };

    int locations[5];
  };

  static const std::array<Location, 5> offsets;
};
extern program_klass program;

struct tex_klass {
  union { struct {
    int uModelView;
    int uProjection;
    int uNormal;
    int uTexMatrix;
    int uTex;
    };

    int locations[5];
  };

  static const std::array<Location, 5> offsets;
};
extern tex_klass tex;

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

struct cursor_klass {
  union { struct {
    int uModelViewProjection;
    int uTex;
    };

    int locations[2];
  };

  static const std::array<Location, 2> offsets;
};
extern cursor_klass cursor;

}