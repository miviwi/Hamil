import re
import sys
import os
import database
import subprocess
import pathlib
import eugene_modules
import eugene_util as util

if os.name == 'nt':
    import eugene_win32 as eugene_sys
elif os.name == 'posix':
    import eugene_sysv as eugene_sys
else:
    raise util.OSUnsupportedError()

from pprint import pprint

class UnknownOperatorError(Exception):
    pass

"""S-expression (list of tokens wrapped in parentheses) -> (x y z...)"""
_COMMAND = re.compile(r"\(\s*((?:[\w\\/\-.\*${}]+\s*)*)\)")

"""Variable substitution -> ${var}"""
_VAR = re.compile(r"\${\s*(\w+)\s*}")

_COMMENT_DELIM = "#"

def _load_script(f):
    script = ""
    for line in f.readlines():
        comment_pos = line.find(_COMMENT_DELIM)
        if comment_pos < 0:
            script += line
            continue

        script += line[:comment_pos]
        script += "\n"

    return script

def _expand_commands(commands, env):
    """Expands 'env' variables in place (mutates 'commands')"""

    def expand_one(match):
        var = match[1]   # First capture group -> stripped variable name
        if var in env:
            return env[var]

        return var

    for cmd in commands:
        for i in range(1, len(cmd)): # Never expand the operator
            expanded = _VAR.sub(expand_one, cmd[i])
            cmd[i] = expanded

# TODO: figure out how to properly create the first component
#       for the path joining
def _fixup_path_args(args):
    args_split = map(lambda arg: arg.split('/'), args)
    args_fixed = map(lambda split: os.path.join(os.path.sep, *split), args_split)

    return list(args_fixed)

_MOVE_COMMAND = None
if os.name == 'nt':
    _MOVE_COMMAND = ['move']
elif os.name == 'posix':
    _MOVE_COMMAND = ['mv', '-u']
else:
    print(f"WARNING: move command unknown for this OS {os.name} - assuming 'mv'")

    _MOVE_COMMAND = ['mv', '-u']

_SUBPROCESS_USE_SHELL = False
if os.name == 'nt':
    _SUBPROCESS_USE_SHELL = True

def _move_if_newer(args, last_command=None, **kwargs):
    if last_command: return

    sources = args[:-1]
    dest = args[-1]
    expanded_args = []
    # Ensure all files in 'args' exist
    #   and expand any wildcards
    for source in sources:
        find_data = None
        try:
            find_data = eugene_sys.FindFiles(source)
        except ValueError:     # no such file(s)
            print(f"(move-if-newer) '{source}' doesn't exist - skipping...")
            return

        for file in find_data:
            expanded_args.append(file['cFileName'])

    command = _MOVE_COMMAND + expanded_args + [dest]
    result = subprocess.run(command,
        # capture_output=True to prevent out-of-order console output
        capture_output=True, shell=_SUBPROCESS_USE_SHELL, encoding='utf-8')

    try:
        result.check_returncode()   # Make sure the command was successfull
    except:
        print(result.stderr)
        #raise    # Rethrow

    print("")
    print(f"{' '.join(command)} {result.stdout}")

def _mkdir(args, **kwargs):
    for arg in args:
        if os.path.exists(arg): continue

        os.mkdir(arg)

_OPERATORS = {
    "move-if-newer": _move_if_newer,
    "mkdir":         _mkdir,
}

def _check_script_freshness(db, script):
    find_data = eugene_sys.FindFiles(script)
    if len(find_data) != 1:
        print(f"no such file `{script}'...")
        print("        ...exiting")
        sys.exit(-1)

    file = find_data[0]
    key, record = file['cFileName'], file['ftLastWriteTime']
    if not db.compareWithRecord(key, record):
        db.invalidate()              # Script file changed - regenerate everything
        db.writeRecord(key, record)

_BUILTIN_VARS = {
    '_PWD': os.getcwd(),
}

def main(db, args):
    _check_script_freshness(db, args[0])

    script = None
    with open(args[0], 'r') as f: # _check_script_freshness() ensures the file exists
        script = _load_script(f)

    env = map(lambda arg: arg.strip().split('='), args[1:])
    env = { name: val for name, val in env }

    # Include the builtin variable definitions
    env.update(_BUILTIN_VARS)

    commands = [ cmd.split() for cmd in _COMMAND.findall(script) if cmd ]
    _expand_commands(commands, env)

    exit_code = -1
    for cmd in commands:
        if not cmd: continue   # Empty expression

        module = cmd[0]
        args = []
        if module not in eugene_modules.MODULES:
            if module not in _OPERATORS:
                raise UnknownOperatorError()

            _OPERATORS[module](cmd[1:], last_command=exit_code)
            continue

        if module != "run":
            args = util.expand_args(cmd[1:])
        else:
            args = cmd[1:]

        # The arguments to commands in *.eugene scripts
        #   are always stored with / (forward slash) used
        #   as path separator. Account for OSes (such
        #   as Windows), which use a different spearator
        #   ex. \ (backslash) used by Windows. 
        args = _fixup_path_args(args)

        exit_code = eugene_modules.MODULES[module](db, args)

    print(f"Last command returned: {exit_code}")
    print("        ...Exiting")

    return 0
