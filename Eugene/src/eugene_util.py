import os
import sys
import database

if os.name == 'nt':
    import eugene_win32 as eugene_sys
elif os.name == 'posix':
    import eugene_sysv as eugene_sys
else:
    raise util.OSUnsupportedError()

class OSUnsupportedError(Exception):
    OS_UNSUPPORTED = "eugene support module not available on this os!"

    def __init__():
        super().__init__(OS_UNSUPPORTED)

HAMIL_PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)),
    '..', '..', 'Hamil')

HAMIL_INCLUDE = os.path.join(HAMIL_PATH, 'include')

_PAD = " "*35
def up_to_date(db, args, pattern):
    """
        The database is updated with the newest ftLastWriteTime value
          automatically by this function
        When 'args' doesn't match any files the query result is always
          'False' i.e. out of date
    """

    any_out_of_date = False
    num_found_files = 0
    for arg in args:
        find_data = None
        try:
            find_data = eugene_sys.FindFiles(pattern(arg))

            num_found_files = num_found_files+len(find_data)
        except ValueError:
            #print(f"couldn't open directory {arg} or no suitable files found within...")
            continue

        for file in find_data:
            key    = file['cFileName']
            record = file['ftLastWriteTime']

            if db.compareWithRecord(key, record): continue   # Up-to-date

            db_record = db.readRecord(key)
            db.writeRecord(key, record)

            old = eugene_sys.GetDateTimeFormat(db_record) if db_record > 0 else "(null)"
            new = eugene_sys.GetDateTimeFormat(record)

            msg = f"`{key}' not up to date"
            msg += _PAD[len(msg):]

            print(f"{msg} ({old} -> {new})...")
            any_out_of_date = True

    # For the query to return a 'True' result
    #  - No database record can be out-of-date
    #  - At LEAST one file in the directories
    #      given in 'args' must've matched the
    #      pattern
    result = (num_found_files > 0) and not any_out_of_date

    if result: print("    ...up to date!")
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
        find_data = eugene_sys.FindFiles(f"{path}{os.path.sep}*")
    except ValueError:
        return []  # path either wasn't a directory or readable - don't return it

    find_data = find_data[2:]   # Don't traverse current and previous directory
    find_data = filter(lambda d: d['bIsDirectory'], find_data)
    find_data = map(lambda d: d['cFileName'], find_data)

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
