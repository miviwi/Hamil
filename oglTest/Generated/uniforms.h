#include <array>
#include <string>
#include <utility>

struct U__ {

  using Location = std::pair<std::string, unsigned>;

  struct program__ {
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
  static program__ program;

  struct tex__ {
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
  static tex__ tex;

  struct font__ {
    union { struct {
      int uModelViewProjection;
      int uAtlas;
      int uColor;
      };

      int locations[3];
    };

    static const std::array<Location, 3> offsets;
  };
  static font__ font;

  struct cursor__ {
    union { struct {
      int uModelViewProjection;
      int uTex;
      };

      int locations[2];
    };

    static const std::array<Location, 2> offsets;
  };
  static cursor__ cursor;

  struct ui__ {
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
  static ui__ ui;

};

extern U__ U;