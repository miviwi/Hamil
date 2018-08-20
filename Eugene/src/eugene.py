import sys
import database
import uniformgen
import resourcegen

_MODULES = {
    "uniformgen":  uniformgen.main,
    "resourcegen": resourcegen.main,
}

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("usage: eugene <module> <directory1> <directory2>...")
        sys.exit(-1)

    module = sys.argv[1]
    args   = sys.argv[2:]

    if module not in _MODULES:
        print(f"<module> must be one of ('{module}' specified):\n")
        for m in _MODULES:
            print(f"        {m}")

        sys.exit(-2)

    exit_code = None
    with database.Database('eugene.db') as db:
        exit_code = _MODULES[module](db, args)

    sys.exit(exit_code)
