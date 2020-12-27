import os
import io
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

from typing import List

_PAD10 = " "*10
_PAD30 = " "*30

Components = List[str]

# Used to output generated *.h/*.cpp (header/source) file contents
FileWriter = io.TextIOWrapper

class ComponentGen:
    def __init__(self, components: Components):
        self._components = components

    def emit(self, header, src):
        pass

def write_hashmap(components: Components, src: FileWriter,
        *, name, key_type, val_type, fmt_key, fmt_val):

    src.write(f"static const std::unordered_map<{key_type}, {val_type}> p_{name} = {{\n")
    for component in components:
        key = fmt_key(component)
        val = fmt_val(component)

        src.write(f"  {{ {key},{_PAD30[len(key):]} {val}{_PAD10[len(val):]} }},\n")
    src.write("};\n")

def write_metaclass_from_type_func_spec(header: FileWriter,
        *, component):

    header.write(f"""
template <>
inline IComponentMetaclass *metaclass_from_type<{component}>()
{{
  return metaclass_from_protoid(ComponentProto::{component});
}}
""")

def write_metaclass_array(components: Components, src: FileWriter):
    src.write(f"""
using IMetaclassPtr = std::unique_ptr<IComponentMetaclass>;
std::array<IMetaclassPtr, ComponentProto::NumProtoIds> p_metaclasses = {{
""")

    for component in components:
        src.write(f"  IMetaclassPtr(new ComponentMetaclass<{component}>()),\n")

    src.write("};\n")

def write_metaclass_from_protoid_func_decl(components: Components, src: FileWriter):
    src.write(f"""
IComponentMetaclass *metaclass_from_protoid(ComponentProtoId protoid)
{{
  assert(protoid < ComponentProto::NumProtoIds);

  return p_metaclasses[protoid].get();
}}
""")

def write_metaclass_class_spec(src: FileWriter, *, component):
    src.write(f"""
template <>
class ComponentMetaclass<{component}> final
    : public ComponentMetaclassBase<{component}>
{{
public:
  ComponentMetaclass()
  {{
    m_metaclass_static.protoid   = ComponentProto::{component};
    m_metaclass_static.data_size = sizeof({component});
    m_metaclass_static.flags     = {component}::Component::flags();
  }}
  
#if 0
  virtual Component *ctor(
      Component *self, const util::IAbstractTuple *args
    ) const final
  {{
    //  =>  new(({component} *)self) {component}();
    /*
    std::apply(
        [=](const auto const&... args_pack) {{
          new(({component} *)self) {component}(args_pack...);
        }},
        args->get<{component}::ConstructorParamPack>()
    );
    */
    
    using ParamPack = {component}::ConstructorParamPack;
    using CtorBinder = BindCtorHelper<ParamPack>;

    std::apply(
        CtorBinder::bind_ctor(({component} *)self),
        args->get<ParamPack>()
    );

    return self;
  }}
#endif
}};
""")

def main(db, args):
    pattern = lambda dir: f"{dir}{os.path.sep}*.h"

    print("\nGenerating Components...")

    if util.up_to_date(db, args, pattern): return 1

    with open('components.h', 'w') as header, open('components.cpp', 'w') as src:
        header.write( \
"""#pragma once

#include <common.h>

#include <hm/hamil.h>
#include <hm/componentmeta.h>
#include <util/staticstring.h>

#include <string>
#include <string_view>

namespace hm {

""")

        src.write( \
"""#include "components.h"

#include <hm/components/all.h>
#include <util/abstracttuple.h>

#include <new>
#include <memory>
#include <unordered_map>
#include <tuple>
#include <functional>
#include <utility>

#include <cassert>

namespace hm {

template <typename ParamPack>
struct BindCtorHelper;

template <typename... Args>
struct BindCtorHelper<std::tuple<Args...>> {

  template <typename T>
  static auto bind_ctor(T *self) -> std::function<void(Args...)>
  {
    return [=](Args... args) {
      new(self) T(std::forward<Args>(args)...);
    };
  }

};

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

        for component in components:
            write_metaclass_class_spec(src, component=component)

        write_metaclass_array(components, src)
        write_metaclass_from_protoid_func_decl(components, src)
        src.write("\n")
    
        write_hashmap(components, src,
            name='str_tag', key_type='std::string', val_type='ComponentTag',
            fmt_key=lambda c: f"\"{c}\"",
            fmt_val=lambda c: f"{c}::tag()"
        )
        src.write("\n")

        write_hashmap(components, src,
            name='tag_protoid', key_type='ComponentTag', val_type='ComponentProtoId',
            fmt_key=lambda c: f"{c}::tag()",
            fmt_val=lambda c: f"ComponentProto::{c}"
        )
        src.write("\n")

        write_hashmap(components, src,
            name='protoid_tag', key_type='ComponentProtoId', val_type='ComponentTag',
            fmt_key=lambda c: f"ComponentProto::{c}",
            fmt_val=lambda c: f"{c}::tag()"
        )

        src.write("""
ComponentTag tag_from_string(const std::string& tag)
{
  auto it = p_str_tag.find(tag);
  if(it == p_str_tag.end()) return "";

  return it->second;
}

ComponentTag tag_from_protoid(ComponentProtoId protoid)
{
  auto it = p_protoid_tag.find(protoid);
  if(it == p_protoid_tag.end()) return "";

  return it->second;
}

ComponentProtoId protoid_from_tag(ComponentTag tag)
{
  auto it = p_tag_protoid.find(tag);
  if(it == p_tag_protoid.end()) return ComponentProto::NullProto;

  return it->second;
}

ComponentProtoId protoid_from_string(const std::string& tag)
{
  // TODO: this function most likely won't be called outside
  //       a debugger/editor context so the implementation
  //       is suboptimal
  //   - add a std::unordered_map<std::string, ComponentProtoId> if necessary

  return protoid_from_tag(tag_from_string(tag));
}

std::string_view protoid_to_str(ComponentProtoId protoid)
{
  const auto tag = tag_from_protoid(protoid);

  return std::string_view(tag.get(), tag.size());
}
""")

        component_protoid = lambda name, protoid:     \
            f"    {name}{_PAD10[len(name):]} = {protoid},\n"

        header.write("""
struct ComponentProto {
  enum Id : ComponentProtoId {
""")

        header.write(component_protoid("NullProto", "~0u"))
        for i, component in enumerate(components):
            header.write(component_protoid(component, i))

        header.write("""
    NumProtoIds,
  };
};

static_assert(
    ComponentProto::NumProtoIds < NumComponentProtoIdBits,
    "Too many Components defined to fit into the Entity Prototype bit-vector!"
);
""")

        for component in components:
            write_metaclass_from_type_func_spec(header, component=component)

        header.write("""

ComponentTag tag_from_string(const std::string& tag);
ComponentTag tag_from_protoid(ComponentProtoId protoid);

ComponentProtoId protoid_from_tag(ComponentTag tag);
ComponentProtoId protoid_from_string(const std::string& tag);

std::string_view protoid_to_str(ComponentProtoId protoid);

}
""")

        src.write("""
}
""")

    return 0
