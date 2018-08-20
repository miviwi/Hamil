import sys
import database
import eugene_util as util
import eugene_win32 as win32

from pprint import pprint

class ParseError(Exception):
    pass

def _gen(header, src, fname):
    def uniform(s):
        return {
            'has_dot': '.' in s,
            'str':     s,
        }

    uniforms = []

    for line in open(fname, 'r').readlines():
        line = line.strip()

        # Strip comments
        comment_pos = line.find('//')
        if comment_pos >= 0: line = line[:comment_pos]

        line = filter(lambda s: s, line.split(','))
        line = map(lambda s: s.strip(), line)

        u = list(map(uniform, line))
        if u: uniforms += u

    fname = fname[fname.rfind('\\')+1:]  # get everything after the last '\\'
    cname = fname[:fname.find('.')]      # get everything before the file extension

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
const std::array<U__::Location, {len(uniforms)}> U__::{cname}__::offsets = {{\n""")

    for (i, u) in enumerate(uniforms):
        src.write(" "*2)
        if u['has_dot']: src.write('//')
        src.write(f"Location{{ \"{u['str']}\", {i} }},\n")

    src.write("};\n")

def main(db, args):
    pattern = lambda dir: f"{dir}\\*.uniform"

    if util.up_to_date(db, args, pattern): return 1

    with open('uniforms.h', 'w') as header, open('uniforms.cpp', 'w') as src:
        header.write(
        """#include <array>
#include <string>
#include <utility>

struct U__ {

  using Location = std::pair<std::string, unsigned>;\n""")

        src.write('#include "uniforms.h"\n')

        for arg in args:
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
                _gen(header, src, fname)

        header.write(
        """};

extern U__ U;""")

        src.write(
        """
U__ U;""")

    db.serialize()

    return 0

if __name__ == "__main__":
    util.exec_module('uniformgen', main)
