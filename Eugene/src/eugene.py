import sys
import database
import uniformgen
import resourcegen
import eugene_util

_MODULES = {
    "uniformgen":  uniformgen.main,
    "resourcegen": resourcegen.main,
}

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("usage: eugene <module> <directory1> <directory2>...")
        sys.exit(-1)

    module = sys.argv[1]

    if module not in _MODULES:
        print(f"<module> must be one of ('{module}' specified):\n")
        for m in _MODULES:
            print(f"        {m}")

        sys.exit(-2)

    args = []
    for arg in sys.argv[2:]:
        args += eugene_util.subdirectories(arg)  # Use += to flatten the lists

    exit_code = None
    with database.Database('eugene.db') as db:
        exit_code = _MODULES[module](db, args)

    sys.exit(exit_code)
