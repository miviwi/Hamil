import uniformgen
import resourcegen
import eugene_script as script

"""
    Dict of module functions with the signature:
        (db: database.Database, args: []) -> int
    That return a status code
"""
MODULES = {
    "uniformgen":  uniformgen.main,
    "resourcegen": resourcegen.main,
    "run":         script.main,
}
