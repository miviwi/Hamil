#include <unordered_map>


namespace U {

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

}