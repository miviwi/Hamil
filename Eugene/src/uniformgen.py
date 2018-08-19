import sys
import database
import eugene_win32 as win32

from pprint import pprint

class ParseError(Exception):
    pass

def gen(header, src, fname):
    uniform = {
        'has_dot': False,
        'str':     '',
    }

    u = uniform.copy()

    uniforms = []

    f = open(fname, 'r')
    while True:
        ch = f.read(1)

        if ch.isspace(): continue

        if ch.isalnum() or ch == '_':
            u['str'] += ch
        elif ch == '.':
            u['str'] += ch
            u['has_dot'] = True
        elif ch == ',':
            uniforms.append(u)

            u = uniform.copy()
        elif ch == '/':
            ch = f.read(1)
            if ch != '/': raise ParseError()

            while ch != '\n': ch = f.read(1)
        elif not ch:
            break

    f.close()

    if u['str']: uniforms.append(u)

    fname = fname[fname.rfind('\\')+1:]  # get everythign after the last '\\'
    cname = fname[:fname.find('.')]

    for u in uniforms:
        print(f"    name: `{u['str']}' has_dot?: {u['has_dot']}")

    header.write(
    f"""
  struct {cname}__ {{
    union {{ struct {{\n""")

    for u in uniforms:
        header.write(" "*6)
        if u['has_dot']: f.write("//")
        header.write(f"int {u['str']};\n")

    header.write(
    f"""      }};

      int locations[{len(uniforms)}];
    }};

    static const std::array<Location, {len(uniforms)}> offsets;
  }};
  static {cname}__ {cname};\n""")

    src.write(
    f"""
U__::{cname}__ U__::{cname};
const std::array<U__::Location, {len(uniforms)}> U__::{cname}))::offsets = {{\n""")

    for (i, u) in enumerate(uniforms):
        src.write(" "*2)
        if u['has_dot']: src.write('//')
        src.write(f"Location{{ \"{u['str']}\", {i} }},\n")

    src.write("};\n")

def main():
    pattern = lambda dir: f"{dir}\\*.uniform"

    db = database.Database('eugene.db')

    up_to_date = True
    for arg in sys.argv[1:]:
        find_data = None
        try:
            find_data = win32.FindFiles(pattern(arg))
        except ValueError:
            print(f"couldn't open directory {arg} or no suitable files found within...")
            continue

        for file in find_data:
            key    = file['cFileName']
            record = file['ftLastWriteTime']

            if not db.compareWithRecord(key, record):
                print(f"`{key}' not up to date ({record})...\n")
                up_to_date = False
                break

    if up_to_date: return 1

    with open('uniforms.h', 'w') as header, open('uniforms.cpp', 'w') as src:
        header.write(
        """#include <array>
#include <string>
#include <utility>

struct U__ {

  using Location = std::pair<std::string, unsigned>;\n""")

        src.write('#include "uniforms.h"\n')

        for arg in sys.argv[1:]:
            find_data = None
            try:
                find_data = win32.FindFiles(pattern(arg))
            except ValueError as e:
                continue

            for file in find_data:
                key    = file['cFileName']
                record = file['ftLastWriteTime']

                fname = f"{arg}\\{key}"

                db.writeRecord(key, record)
                gen(header, src, fname)

        header.write(
        """};

extern U__ U;""")

        src.write(
        """
U__ U;""")

    db.serialize()

    return 0

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: eugene <directory1> <directory2>...")
        sys.exit(-1)

    sys.exit(main())
