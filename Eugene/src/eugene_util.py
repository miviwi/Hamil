import os
import sys
import database

import platform
if platform.system() == 'Windows':
    import eugene_win32 as eugene_sys
elif platform.system() == 'Linux':
    import eugene_sysv as eugene_sys

HAMIL_PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)),
    '..', '..', 'Hamil')

HAMIL_INCLUDE = os.path.join(HAMIL_PATH, 'include')

_PAD = " "*35
def up_to_date(db, args, pattern):
    """
        The database is updated with the newest ftLastWriteTime value
          automatically by this function
    """

    result = True
    for arg in args:
        find_data = None
        try:
            find_data = eugene_sys.FindFiles(pattern(arg))
        except ValueError:
            #print(f"couldn't open directory {arg} or no suitable files found within...")
            continue

        for file in find_data:
            key    = file['cFileName']
            record = file['ftLastWriteTime']

            if not db.compareWithRecord(key, record):
                db_record = db.readRecord(key)
                db.writeRecord(key, record)

                old = eugene_sys.GetDateTimeFormat(db_record) if db_record > 0 else "(null)"
                new = eugene_sys.GetDateTimeFormat(record)

                msg = f"`{key}' not up to date"
                msg += _PAD[len(msg):]

                print(f"{msg} ({old} -> {new})...")
                result = False

    if result:
        print("    ...up to date!")

    return result

def exec_module(name, main):
    if len(sys.argv) < 2:
        print(f"usage: python {name} <directory1> <directory2>...")
        sys.exit(-1)

    with database.Database('eugene.db') as db:
        args = sys.argv[1:]
        exit_code = main(db, args)

        sys.exit(exit_code)

def subdirectories(path):
    find_data = None
    try:
        find_data = eugene_sys.FindFiles(f"{path}\\*")
    except ValueError:
        return []  # path either wasn't a directory or readable - don't return it

    find_data = find_data[2:]   # Don't traverse current and previous directory
    find_data = filter(lambda d: d['bIsDirectory'], find_data)
    find_data = map(lambda d: f"{path}\\{d['cFileName']}", find_data)

    subdirs = [path]
    for d in find_data:
        subdirs += subdirectories(d)

    return subdirs

def expand_args(args):
    expanded = []
    for arg in args:
        if os.path.isdir(arg):
            expanded += subdirectories(arg) # Use += to flatten the list
        else:
            expanded.append(arg)

    return expanded
