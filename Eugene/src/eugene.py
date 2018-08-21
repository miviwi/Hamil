import sys
import database
import eugene_modules
import eugene_util

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("usage: eugene <module> <directory1> <directory2>...")
        sys.exit(-1)

    module = sys.argv[1]

    if module not in eugene_modules.MODULES:
        print(f"<module> must be one of ('{module}' specified):\n")
        for m in _MODULES:
            print(f"        {m}")

        sys.exit(-2)

    args = []
    if module != "run":
        args = eugene_util.expand_args(sys.argv[2:])
    else:
        args = sys.argv[2:]

    exit_code = None
    with database.Database('eugene.db') as db:
        exit_code = eugene_modules.MODULES[module](db, args)

    sys.exit(exit_code)
