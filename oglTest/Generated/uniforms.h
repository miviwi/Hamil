#include <unordered_map>


namespace U {

struct cursor_klass {
  union { struct {
    int uModelView;
    int uProjection;
    int uTex;
    };

    int locations[3];
  };

  static const std::unordered_map<std::string, unsigned> offsets;
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

  static const std::unordered_map<std::string, unsigned> offsets;
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

  static const std::unordered_map<std::string, unsigned> offsets;
};
extern program_klass program;

struct ui_klass {
  union { struct {
    int uProjection;
    };

    int locations[1];
  };

  static const std::unordered_map<std::string, unsigned> offsets;
};
extern ui_klass ui;

}