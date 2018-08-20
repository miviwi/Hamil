import sys
import database
import eugene_win32 as win32

def up_to_date(db, args, pattern):
    result = True
    for arg in args:
        find_data = None
        try:
            find_data = win32.FindFiles(pattern(arg))
        except ValueError:
            print(f"couldn't open directory {arg} or no suitable files found within...")
            continue

        for file in find_data:
            key    = file['cFileName']
            record = file['ftLastWriteTime']

            if not db.compareWithRecord(key, record):
                db_record = db.readRecord(key)

                print(f"`{key}' not up to date ({db_record} -> {record})...\n")
                result = False
                break

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
        find_data = win32.FindFiles(f"{path}\\*")
    except ValueError:
        return []

    find_data = find_data[2:]   # Don't traverse current and previous directory
    find_data = filter(lambda d: d['bIsDirectory'], find_data)
    find_data = map(lambda d: f"{path}\\{d['cFileName']}", find_data)

    subdirs = [path]
    for d in find_data:
        subdirs += subdirectories(d)

    return subdirs
