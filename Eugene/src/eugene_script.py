import re
import sys
import os
import database
import subprocess
import eugene_modules
import eugene_util

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

def _move_if_newer(args, last_command=None, **kwargs):
    if last_command: return

    result = subprocess.run(["move"] + args,
        # capture_output=True to prevent out-of-order console output
        capture_output=True, shell=True, encoding='utf-8')

    print("")
    print(result.stdout)

def _mkdir(args, **kwargs):
    for arg in args:
        if os.path.exists(arg): continue

        os.mkdir(arg)

_OPERATORS = {
    "move-if-newer": _move_if_newer,
    "mkdir":         _mkdir,
}

def main(db, args):
    script = None
    try:
        with open(args[0], 'r') as f:
            script = _load_script(f)
    except FileNotFoundError:
        print(f"no such file `{args[0]}'...")
        print("        ...exiting")
        sys.exit(-1)

    env = map(lambda arg: arg.strip().split('='), args[1:])
    env = { name: val for name, val in env }

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
            args = eugene_util.expand_args(cmd[1:])
        else:
            args = cmd[1:]

        exit_code = eugene_modules.MODULES[module](db, args)

    print(f"Last command returned: {exit_code}")
    print("        ...Exiting")

    return 0
