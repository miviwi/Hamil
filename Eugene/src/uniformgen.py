import sys
import os
import cxx
import database
import eugene_util as util

if os.name == 'nt':
    import eugene_win32 as eugene_sys
elif os.name == 'posix':
    import eugene_sysv as eugene_sys
else:
    raise util.OSUnsupportedError()

from pprint import pprint

class ParseError(Exception):
    pass

def _gen(header, src, fname):
    tu = cxx.CxxTranslationUnit(fname, [util.HAMIL_INCLUDE])

    Uniforms = tu.namespace('gx').class_('Uniforms')
    UniformsName = Uniforms.class_('Name')

    uniform_classes = []
    for ns in tu.namespaces.values():
        uniform_classes += filter(
            lambda c: Uniforms.is_base_of(c), ns.classes.values()
        )

    uniforms = {}
    for u in uniform_classes:
        name = list(filter(
            lambda c: cxx.CxxClass(c) == UniformsName, u.fields
        ))

        if len(name) != 1:
            raise ParseError('No program name or more than one specified!')

        uniforms[name[0].spelling] = u

    for cname, klass in uniforms.items():
        print(f"Generating: {klass.name}({cname})")

        u = filter(lambda c: cxx.CxxClass(c) != UniformsName, klass.fields)
        _gen_one(header, src, cname, list(u))

def _gen_one(header, src, cname, uniforms):
    num = len(uniforms)

    header.write(
    f"""
  struct {cname}__ {{
    union {{ struct {{\n""")

    for u in uniforms:
        typename = u.type.get_declaration().spelling
        print(f"{' '*8}{typename if typename else u.type.spelling} {u.spelling};")

        header.write(" "*6)
        header.write(f"int {u.spelling};\n")

    header.write(
    f"""      }};

      int locations[{num}];
    }};

    static const std::array<Location, {num}> offsets;
  }};
  static {cname}__ {cname};\n""")

    src.write(
    f"""
U__::{cname}__ U__::{cname};
const std::array<U__::Location, {num}> U__::{cname}__::offsets = {{\n""")

    for i, u in enumerate(uniforms):
        src.write(" "*2)
        src.write(f"Location{{ \"{u.spelling}\", {i} }},\n")

    src.write("};\n")

def main(db, args):
    pattern = lambda f: f

    print("\nGenerating Uniforms...")

    if util.up_to_date(db, args, pattern): return 1

    with open('uniforms.h', 'w') as header, open('uniforms.cpp', 'w') as src:
        header.write(
        """#pragma once

#include <array>
#include <string>
#include <utility>

struct U__ {

  using Location = std::pair<std::string, unsigned>;\n""")

        src.write('#include "uniforms.h"\n')

        for arg in args:
            find_data = None
            try:
                find_data = eugene_sys.FindFiles(pattern(arg))
            except ValueError as e:
                continue

            for file in find_data:
                key    = file['cFileName']
                record = file['ftLastWriteTime']

                db.writeRecord(key, record)
                _gen(header, src, key)

        header.write(
        """};

extern U__ U;""")

        src.write(
        """
U__ U;""")

    return 0

if __name__ == "__main__":
    util.exec_module('uniformgen', main)
