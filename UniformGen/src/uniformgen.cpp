#define _CRT_SECURE_NO_WARNINGS 1

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <memory>
#include <vector>
#include <string>
#include <utility>

void gen(FILE *header, FILE *src, const char *fname)
{
  struct Uniform {
    bool has_dot = false;
    std::string str;
  };

  Uniform u;
  std::vector<Uniform> uniforms;

  FILE *fp = fopen(fname, "rb");

  while(!feof(fp)) {
    char ch = fgetc(fp);

    if(!isspace(ch)) {
      if(isalnum(ch) || ch == '_') {
        u.str += ch;
      } else if(ch  == '.') {
        u.str += ch;
        u.has_dot = true;
      } else if(ch == ',') {
        uniforms.push_back(std::move(u));

        u = Uniform();
      } else if(ch == '/') {
        ch = fgetc(fp);
        if(ch != '/') goto error;

        while(ch != '\n') ch = fgetc(fp);
      } else if(ch == EOF) {
        break;
      } else {
error:
        printf("stray character `%c' (%d) at file offset %ld!\n", ch, ch, ftell(fp));
        exit(-2);
      }
    }
  }

  fclose(fp);

  if(u.str.size()) uniforms.push_back(u);

  fname = strrchr(fname, '\\')+1;
  std::string cname(fname, strchr(fname, '.')-fname);

  for(auto& u : uniforms) {
    printf("name: `%s' has_dot?: %d\n", u.str.c_str(), u.has_dot);
  }

  fprintf(header,
    "struct %s_klass {\n"
    "  union { struct {\n",

    cname.c_str()
  );

  //Generate Class
  for(auto& u : uniforms) {
    fprintf(header, "    %sint %s;\n", u.has_dot ? "//" : "", u.str.c_str());
  }

  fprintf(header,
    "    };\n"
    "\n"
    "    int locations[%d];\n"
    "  };\n"
    "\n"
    "  static const std::unordered_map<std::string, unsigned> offsets;\n"
    "};\n"
    "extern %s_klass %s;\n"
    "\n",

    (int)uniforms.size(), cname.c_str(), cname.c_str()
  );

  fprintf(src,
    "\n"
    "%s_klass %s;\n"
    "const std::unordered_map<std::string, unsigned> %s_klass::offsets = {\n",

    cname.c_str(), cname.c_str(), cname.c_str()
  );

  for(int i = 0; i < uniforms.size(); i++) {
    const Uniform& u = uniforms[i];

    fprintf(src, "  %s{ \"%s\", %u },\n", u.has_dot ? "//" : "", u.str.c_str(), i);
  }

  fprintf(src,
    "};\n"
    "\n"
  );
}

int main(int argc, char *argv[])
{
  if(argc < 2) {
    puts("usage: uniformgen <directory1> <directory2>...");
    exit(-1);
  }

  FILE *header = fopen("uniforms.h", "wb"),
    *src = fopen("uniforms.cpp", "wb");

  fprintf(header,
    "#include <unordered_map>\n"
    "\n"
    // "#include <GL/glew.h>\n"
    "\n"
    "namespace U {\n"
    "\n"
  );

  fprintf(src,
    "#include \"uniforms.h\"\n"
    "\n"
    "namespace U {\n"
  );

  for(int i = 1; i < argc; i++) {
    auto pattern = argv[i] + std::string("\\*.uniform");

    WIN32_FIND_DATAA find_data;
    auto handle = FindFirstFileA(pattern.c_str(), &find_data);
    do {
      printf("file: %s\n", find_data.cFileName);

      auto fname = argv[i] + std::string("\\") + find_data.cFileName;

      gen(header, src, fname.c_str());
    } while(FindNextFileA(handle, &find_data));

    FindClose(handle);
  }

  fprintf(header, "}");
  fprintf(src, "}");

  fclose(header);
  fclose(src);
}