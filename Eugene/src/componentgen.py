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

def main(db, args):
    pattern = lambda dir: f"{dir}{os.path.sep}*.h"

    print("\nGenerating Components...")

    if util.up_to_date(db, args, pattern): return 1

    with open('components.h', 'w') as header, open('components.cpp', 'w') as src:
        header.write(
        """#pragma once

#include <hm/componentstore.h>

#include <util/staticstring.h>

#include <string>

namespace hm {

""")

        src.write(
        """#include "components.h"

#include <hm/components/all.h>

#include <unordered_map>

namespace hm {

static const std::unordered_map<std::string, Component::Tag> p_tags = {
""")
        tu = cxx.CxxTranslationUnit(
            # Need to pick a TranslationUnit with #include <hm/compoents/all.h>
            os.path.join(util.HAMIL_PATH, 'include', 'hm', 'components', 'all.h'),#'componentstore.cpp'),
            [util.HAMIL_INCLUDE]
        )

        hm = tu.namespace('hm')

        Component = hm.class_('Component')
        component_classes = filter(
            lambda c: Component.is_base_of(c[1]), hm.classes.items()
        )

        # c[0] is the non-namespace qualified class name
        components = list(map(lambda c: c[0], component_classes))

        print(f"resolved components: {components}")

        for component in components:
            header.write(f"struct {component};\n")
            src.write(f"  {{ \"{component}\", {component}::tag() }},\n")

        src.write(
        """};

util::StaticString tag_from_string(const std::string& tag)
{
    auto it = p_tags.find(tag);
    if(it == p_tags.end()) return "";

    return it->second;
}
""")

        header.write(
        """
class ComponentStore : public ComponentStoreBase<""")

        for i, component in enumerate(components):
            header.write(f"\n{' '*4}{component}")

            if i+1 < len(components): header.write(',')

        header.write(
        """
> { };

util::StaticString tag_from_string(const std::string& tag);

}
""")

        src.write(
        """
}
""")

    return 0
