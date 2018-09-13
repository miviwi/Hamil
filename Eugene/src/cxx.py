import os
import clang.cindex

from pprint import pprint

_EUGENE_PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)), '..')

clang.cindex.Config.set_library_path(
    os.path.join(_EUGENE_PATH, 'extern', 'LLVM', 'win64')
)

class CxxClass:
    def __init__(self, decl: clang.cindex.Cursor):
        self._decls = list(decl.type.get_declaration().get_children())

        self._name    = decl.type.spelling
        self._bases   = self._make_bases()
        self._fields  = self._make_fields()
        self._methods = self._make_methods()
        self._classes = self._make_classes()

    def __str__(self):
        return "class " + self._name + self._bases_str()

    def __repr__(self):
        return f"<CxxClass {self._name}{self._bases_str()}>"

    def __eq__(self, other):
        if type(other) is not type(self):
            return False

        return self._name == other._name

    @property
    def name(self) -> str:
        return self._name

    @property
    def fields(self) -> list:
        return self._fields

    @property
    def methods(self) -> list:
        return self._methods

    @property
    def decls(self) -> list:
        return self._decls

    @property
    def bases(self) -> list:
        return self._bases

    @property
    def classes(self) -> dict:
        """Nested class definitions"""
        return self._classes

    def class_(self, name: str):
        return self._classes.get(name)

    def is_base_of(self, child) -> bool:
        if type(child) is not type(self):
            raise ValueError("'child' must be a CxxClass")

        return self._name in map(lambda b: b._name, child._bases)

    def _make_bases(self):
        specifiers = map(
            lambda s: CxxClass(s.type.get_declaration()), filter(
                lambda s: s.kind == clang.cindex.CursorKind.CXX_BASE_SPECIFIER,
                self._decls
            )
        )
        return list(specifiers)

    def _make_fields(self):
        fields = filter(
            lambda f: f.kind == clang.cindex.CursorKind.FIELD_DECL, self._decls
        )
        return list(fields)

    def _make_methods(self):
        methods = filter(
            lambda m: m.kind == clang.cindex.CursorKind.CXX_METHOD, self._decls
        )
        return list(methods)

    _CLASS_CURSOR_KINDS = {
        clang.cindex.CursorKind.CLASS_DECL,
        clang.cindex.CursorKind.STRUCT_DECL,
    }
    def _make_classes(self):
        classes = map(
            lambda c: (c.spelling, CxxClass(c.type.get_declaration())), filter(
                lambda c: c.kind in self._CLASS_CURSOR_KINDS,
                self._decls
            )
        )
        return { name: decl for name, decl in classes }

    def _bases_str(self):
        bases = ','.join(map(lambda b: b._name, self._bases))
        return f" : {bases}" if bases else ""

class CxxNamespace:
    def __init__(self, name: str, decls: clang.cindex.Cursor):
        self._decls = list(decls)

        self._funcs = self._extract_kinds({
            clang.cindex.CursorKind.FUNCTION_DECL,
            #clang.cindex.CursorKind.FUNCTION_TEMPLATE,
        })
        self._classes = self._extract_kinds({
            clang.cindex.CursorKind.CLASS_DECL, clang.cindex.CursorKind.STRUCT_DECL,
            #clang.cindex.CursorKind.CLASS_TEMPLATE,
        }, kinds_type=CxxClass)

    @property
    def functions(self) -> dict:
        return self._funcs

    @property
    def classes(self) -> dict:
        return self._classes

    def class_(self, name: str) -> CxxClass:
        return self._classes.get(name)

    def _extract_kinds(self, kinds: set, kinds_type=None) -> dict:
        result = filter(lambda d: d.kind in kinds, self._decls)
        result = map(lambda d: (d.spelling, d), result)

        if kinds_type is None:
            return { name: decl for name, decl in result }

        return { name: kinds_type(decl) for name, decl in result }


class CxxTranslationUnit:
    def __init__(self, path: str, include: list):
        self._index = clang.cindex.Index.create()
        self._tu = self._index.parse(
            path, args=["-I" + ','.join(include)]
        )

        ns_decls = filter(
            lambda c: c.kind == clang.cindex.CursorKind.NAMESPACE,
            self._tu.cursor.get_children()
        )
        namespaces = { }
        for n in ns_decls:
            decls = namespaces.get(n.spelling, [])
            decls += list(n.get_children())

            namespaces[n.spelling] = decls

        self._namespaces = {
            name: CxxNamespace(name, decls) for name, decls in namespaces.items()
        }
        self._namespaces[''] = CxxNamespace('', self._tu.cursor.get_children())

    @property
    def namespaces(self):
        return self._namespaces

    @property
    def global_(self):
        return self.namespace('')

    def namespace(self, name: str) -> CxxNamespace:
        return self._namespaces.get(name)
