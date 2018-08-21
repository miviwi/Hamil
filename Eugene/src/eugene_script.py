import re
import sys
import database
import eugene_modules
import eugene_util

from pprint import pprint

# Regex for an s-expression (list of tokens wrapped in parentheses)
_COMMAND = re.compile(r"\(\s*((?:[\w\\/\-.\*\+]+\s*)*)\)")

def _load_script(f):
    script = ""
    for line in f.readlines():
        comment_pos = line.find('#')
        if comment_pos < 0:
            script += line
            continue

        script += line[:comment_pos]
        script += "\n"

    return script

def _expand_commands(commands, env):
    """Expands 'env' variables in place (mutates 'commands')"""

    for cmd in commands:
        for i in range(1, len(cmd)): # Never expand the operator
            expanded = map(lambda x: env[x] if x in env else x, cmd[i].split('+'))
            cmd[i] = ''.join(expanded)

def main(db, args):
    script = None
    try:
        with open(args[0], 'r') as f:
            script = _load_script(f)
    except FileNotFoundError:
        print(f"no such file `{args[0]}'...")
        print("        ...exiting")
        sys.exit(-1)

    env = map(lambda arg: arg.split('='), args[1:])
    env = { name: val for name, val in env }

    print(f"script:\n{script}\nenv:")
    pprint(env)

    commands = [ cmd.split() for cmd in _COMMAND.findall(script) if cmd ]
    _expand_commands(commands, env)
    print(f"commands:")
    pprint(commands)

    exit_code = -1
    for cmd in commands:
        if not cmd: continue

        module = cmd[0]
        args = []
        if module != "run":
            args = eugene_util.expand_args(cmd[1:])
        else:
            args = cmd[1:]

        exit_code = eugene_modules.MODULES[module](db, args)

    return exit_code
