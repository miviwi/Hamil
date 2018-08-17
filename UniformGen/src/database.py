import struct

_GOOD_HEADER = b'00DB'

_NumRecords = 'I'
_KeyLength  = 'H'
_Record     = 'Q'

class InvalidDatabaseError(Exception):
    pass

class Database:
    def __init__(self, fname):
        self.fname = fname
        try:
            with open(fname, 'rb') as f: self._read_db(f)
        except FileNotFoundError:
            print(f"No database `{fname}' found...\n")

        print(f"Successfully loaded Database `{fname}'...")

    def serialize(self):
        with open(fname, 'wb') as f: self._write_db(f)

    def _read_db(self, f):
        header = f.read(len(_GOOD_HEADER))
        if header != _GOOD_HEADER:
            raise InvalidDatabaseError

        self.records = {}

        num_records = self._unpack(_NumRecords)
        for i in range(num_records):
            (key, record) = self._unpack_record(f)
            self.records[key] = record

    def _write_db(self, f):
        f.write(_GOOD_HEADER)

        for (key, record) in self.records.items:
          self._pack_record(f, key, record)

    @staticmethod
    def _unpack(f, fmt):
        buf = f.read(struct.calcsize(fmt))
        (val,) = struct.unpack(fmt, buf) # struct.unpack returns a tuple

        return val

    @classmethod
    def _unpack_record(cls, f):
        key_length = cls._unpack(_KeyLength)
        key = f.read(key_length)

        record = cls._unpack(_Record)

        return (key, record)

    @classmethod
    def _pack_record(cls, f, key, record):
        buf = struct.pack(f"{_KeyLength}s{_Record}", (len(key), key, record))
        f.write(buf)
