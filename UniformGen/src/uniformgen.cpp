#include "database.h"

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
    printf("    name: `%s' has_dot?: %d\n", u.str.c_str(), u.has_dot);
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
    "  static const std::array<Location, %d> offsets;\n"
    "};\n"
    "extern %s_klass %s;\n"
    "\n",

    (int)uniforms.size(), (int)uniforms.size(), cname.c_str(), cname.c_str()
  );

  fprintf(src,
    "\n"
    "%s_klass %s;\n"
    "const std::array<Location, %d> %s_klass::offsets = {\n",

    cname.c_str(), cname.c_str(), (int)uniforms.size(), cname.c_str()
  );

  for(int i = 0; i < uniforms.size(); i++) {
    const Uniform& u = uniforms[i];

    fprintf(src, "  %sLocation{ \"%s\", %u },\n", u.has_dot ? "//" : "", u.str.c_str(), i);
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

  Database db("uniformgen.db");

  // Compare against Database
  bool up_to_date = true;
  for(int i = 1; i < argc; i++) {
    auto pattern = argv[i] + std::string("\\*.uniform");

    WIN32_FIND_DATAA find_data;
    auto handle = FindFirstFileA(pattern.c_str(), &find_data);
    if(handle == INVALID_HANDLE_VALUE) {
      printf("\ncouldn't open directory %s, aborting...\n", argv[i]);
      return -2;
    }

    do {
      Key key = find_data.cFileName;
      Record record = find_data.ftLastWriteTime;

      if(!db.compareWithRecord(key, record)) {
        printf("`%s' not up to date (%llu)...\n\n", find_data.cFileName, get_record(record));
        up_to_date = false;
        break;
      }
    } while(FindNextFileA(handle, &find_data));

    FindClose(handle);

    if(!up_to_date) break;
  }

  if(up_to_date) return 1;

  FILE *header = fopen("uniforms.h", "wb"),
    *src = fopen("uniforms.cpp", "wb");

  fprintf(header,
    "#include <array>\n"
    "#include <string>\n"
    "#include <utility>\n"
    //"\n"
    // "#include <GL/glew.h>\n"
    "\n"
    "namespace U {\n"
    "\n"
    "using Location = std::pair<std::string, unsigned>;\n"
    "\n"
  );

  fprintf(src,
    "#include \"uniforms.h\"\n"
    "\n"
    "namespace U {\n"
  );

  // Generate
  for(int i = 1; i < argc; i++) {
    auto pattern = argv[i] + std::string("\\*.uniform");

    WIN32_FIND_DATAA find_data;
    auto handle = FindFirstFileA(pattern.c_str(), &find_data);
    do {
      Key key = find_data.cFileName;
      Record record = find_data.ftLastWriteTime;

      auto fname = argv[i] + std::string("\\") + find_data.cFileName;

      db.writeRecord(key, record);
      gen(header, src, fname.c_str());
    } while(FindNextFileA(handle, &find_data));

    FindClose(handle);
  }

  fprintf(header, "}");
  fprintf(src, "}");

  fclose(header);
  fclose(src);

  db.serialize();

  return 0;
}