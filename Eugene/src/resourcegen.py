import sys
import os
import database
import yaml
import eugene_util as util

if os.name == 'nt':
    import eugene_win32 as eugene_sys
elif os.name == 'posix':
    import eugene_sysv as eugene_sys
else:
    raise util.OSUnsupportedError()

from pprint import pprint

class ResourceGen:
    def __init__(self, docs):
        self.paths = { doc['tag']: {} for doc in docs }

        for tag in self.paths:
            node = self.paths[tag]
            node['$'] = []

            self._init_paths(docs, tag=tag, node=node)

    def _init_paths(self, docs, tag, node):
        """
            self.paths: each 'dict' represents a directory,
              and has a special '$' key which holds a 'list'
              of all the files
        """

        for doc in docs:
            if doc['tag'] != tag: continue

            self._init_path(doc, node=node)

    def _init_path(self, doc, path=None, *, node=None):
        name = doc['name']
        if path is None:
            path = doc['path']

        if not path:
            n = (doc['name'], hex(doc['guid']))
            node['$'].append(n)
            return

        slash_pos = path.find('/', 1)
        if slash_pos < 0: slash_pos = len(path)

        dir = path[1:slash_pos] # get everything before the first /
                                # and skip the leading one
        if dir not in node: node[dir] = { '$': [] } # add the directory if it doesn't exist

        self._init_path(doc, path[slash_pos:], node=node[dir])

    _PAD = " "*20
    def _emit_node(self, node, header, src, level):
        name, guid = node
        name += self._PAD[len(name) + level*2:] # Align all GUIDs nicely :)
        header.write(f"{'  '*level}static constexpr size_t {name} = {guid};\n")

    def _emit_ids(self, ids, header, src, level):
        header.write(f"\n{'  '*level}// The order of the ids is UNDEFINED!\n")
        header.write(f"{'  '*level}static constexpr std::array<size_t, {len(ids)}> ids = {{\n")

        for _, guid in ids:
            header.write(f"{'  '*(level+1)}{guid},\n")

        header.write(f"{'  '*level}}};\n")

    def _emit_path(self, path, header, src, level=1):
        indent = lambda: header.write("  "*level)

        for key, val in path.items():
            if key == '$':
                for node in val: self._emit_node(node, header, src, level)
                continue

            indent()
            header.write(f"struct {key}__ {{\n")

            self._emit_path(val, header, src, level+1)
            if '$' in val: self._emit_ids(val['$'], header, src, level+1)

            indent()
            header.write(f"}} {key};\n")

            if '$' not in path: header.write("\n")

    def emit(self, header, src):
        print("    ...emitting")
        pprint(self.paths)

        self._emit_path(self.paths, header, src)

def main(db, args):
    pattern = lambda dir: f"{dir}{os.path.sep}*.meta"

    print("\nGenerating Resources...")

    if util.up_to_date(db, args, pattern): return 1

    # Fix yaml.constructor.ConstructorError (ignore tags)
    yaml.add_multi_constructor('', lambda loader, tag, node: node.value)

    with open('resources.h', 'w') as header, open('resources.cpp', 'w') as src:
        header.write(
        """#pragma once

#include <cstddef>
#include <array>

struct R__ {

""")

        src.write('#include "resources.h"\n')

        docs = []
        for arg in args:
            find_data = None
            try:
                find_data = eugene_sys.FindFiles(pattern(arg))
            except ValueError:
                continue

            for file in find_data:
                fname = file['cFileName']
                with open(fname, 'r') as f: docs.append(yaml.load(f, Loader=yaml.Loader))

        r = ResourceGen(docs)
        r.emit(header, src)

        header.write(
"""};

extern R__ R;""")

        src.write(
        """
R__ R;""")

    return 0

if __name__ == "__main__":
    util.exec_module('resourcegen', main)
