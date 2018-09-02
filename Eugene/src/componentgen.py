import re
import sys
import database
import eugene_win32 as win32
import eugene_util as util

_COMPONENT_DELIM = "!$"

_COMPONENT = re.compile(r'\s*(?:struct|class)\s+(\w+)')

def _get_components(f):
    component_incoming = False
    components = []
    for line in f.readlines():
        if not line.strip(): continue # Skip any empty lines

        if component_incoming:
            component = _COMPONENT.match(line)
            if component is None: continue # No definition yet...

            # 1st capture group == ComponentClassName
            components.append(component[1])

            component_incoming = False
            continue

        component_pos = line.find(_COMPONENT_DELIM)
        if component_pos < 0: continue

        component_incoming = True # Next 'struct' definition will be a component

    return components

def main(db, args):
    pattern = lambda dir: f"{dir}\\*.h"

    if util.up_to_date(db, args, pattern): return 1

    with open('components.h', 'w') as header, open('components.cpp', 'w') as src:
        header.write(
        """#include <game/componentstore.h>

namespace game {

""")

        src.write(
        """#include "components.h"

""")

        components = []
        for arg in args:
            find_data = None
            try:
                find_data = win32.FindFiles(pattern(arg))
            except ValueError:
                continue

            for file in find_data:
                fname = f"{arg}\\{file['cFileName']}"
                with open(fname, 'r') as f: components += _get_components(f)

        for component in components:
            header.write(f"struct {component};\n")

        header.write(
        """
using ComponentStore = ComponentStoreBase<""")

        for i, component in enumerate(components):
            header.write(f"\n{' '*8}{component}")

            if i+1 < len(components): header.write(',')

        header.write(
        """
    >;

}
""")

    return 0
