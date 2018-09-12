import os
import cxx
import database
import eugene_win32 as win32
import eugene_util as util

from pprint import pprint

def main(db, args):
    pattern = lambda dir: f"{dir}\\*.h"

    if util.up_to_date(db, args, pattern): return 1

    with open('components.h', 'w') as header, open('components.cpp', 'w') as src:
        header.write(
        """#include <hm/componentstore.h>

namespace hm {

""")

        src.write(
        """#include "components.h"

""")
        tu = cxx.CxxTranslationUnit(
            # Need to pick a TranslationUnit with #include <hm/compoents/all.h>
            os.path.join(util.HAMIL_PATH, 'src', 'hm', 'componentstore.cpp'),
            args
        )

        hm = tu.namespace('hm')

        Component = hm.class_('Component')
        component_classes = filter(
            lambda c: Component.is_base_of(c[1]), hm.classes.items()
        )

        # c[0] is the non-namespace qualified class name
        components = list(map(lambda c: c[0], component_classes))

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
