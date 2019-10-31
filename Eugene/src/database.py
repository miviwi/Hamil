import os
import os.path
import struct

_GOOD_HEADER = b'00DB'

_NumRecords = 'I'
_KeyLength  = 'H'
_Record     = 'Q'

class InvalidDatabaseError(Exception):
    pass

class DatabaseAlreadyOpenError(Exception):
    pass

class Database:
    def __init__(self, fname):
        self.fname   = fname
        self.records = {}

        self._lock_db()

        try:
            with open(fname, 'rb') as f: self._read_db(f)
        except FileNotFoundError:
            print(f"No database `{fname}' found...")
        else:
            print(f"Successfully loaded Database `{fname}'...")

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    def close(self):
        self.serialize()
        self._unlock_db()

    def serialize(self):
        with open(self.fname, 'wb') as f: self._write_db(f)

        return self

    def compareWithRecord(self, key, record):
        entry = self.records.get(key.encode(), 0)
        return entry >= record

    def readRecord(self, key):
        return self.records.get(key.encode(), 0)

    def writeRecord(self, key, record):
        db_record = self.readRecord(key)

        # Deal with win32 FindFirstFile sometimes returning
        #   dates which are in the future...
        if record > db_record: self.records[key.encode()] = record

    def invalidate(self):
        self.records = {}

    def _lockfile(self):
        return f"{self.fname}.lock"

    def _lock_db(self):
        if os.path.isfile(self._lockfile()):
            raise DatabaseAlreadyOpenError()

        self.lock = open(self._lockfile(), 'a')

    def _unlock_db(self):
        self.lock.close()
        self.lock = None

        os.remove(self._lockfile())

    def _read_db(self, f):
        header = f.read(len(_GOOD_HEADER))
        if header != _GOOD_HEADER:
            raise InvalidDatabaseError()

        num_records = self._unpack(f, _NumRecords)
        for i in range(num_records):
            (key, record) = self._unpack_record(f)
            self.records[key] = record

    def _write_db(self, f):
        f.write(_GOOD_HEADER)

        buf = struct.pack(_NumRecords, len(self.records))
        f.write(buf)

        for (key, record) in self.records.items():
            self._pack_record(f, key, record)

    @staticmethod
    def _unpack(f, fmt):
        buf = f.read(struct.calcsize(fmt))
        (val,) = struct.unpack(fmt, buf) # struct.unpack returns a tuple

        return val

    @classmethod
    def _unpack_record(cls, f):
        key_length = cls._unpack(f, _KeyLength)
        key = f.read(key_length)

        record = cls._unpack(f, _Record)

        return (key, record)

    @classmethod
    def _pack_record(cls, f, key, record):
        buf = struct.pack(_KeyLength, len(key))
        f.write(buf)

        f.write(key)

        buf = struct.pack(_Record, record)
        f.write(buf)
