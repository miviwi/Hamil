#pragma once

#include <cstdint>

namespace win32 {

class FileView;

class File {
public:
  using Size = unsigned long;

  enum Access {
    AccessNone = 0,
    AccessAll  = ~0,

    Read    = (1<<0),
    Write   = (1<<1),
    Execute = (1<<2),
  };

  enum Share {
    ShareNone  = 0,
    ShareRead  = (1<<0),
    ShareWrite = (1<<1),
  };

  enum OpenMode {
    CreateAlways,
    CreateNew,
    OpenAlways,
    OpenExisting,
    TruncateExisting,
  };

  enum Protect {
    ProtectNone = 0,
    ProtectRead    = (1<<0),
    ProtectWrite   = (1<<1),
    ProtectExecute = (1<<2),

    ProtectReadWrite = ProtectRead|ProtectWrite,
    ProtectExecuteRead = ProtectExecute|ProtectRead,
    ProtectExecuteReadWrite = ProtectExecute|ProtectRead|ProtectWrite,
  };

  File(const char *path, Access access, Share share, OpenMode open);
  File(const char *path, Access access);
  File(const File& other) = delete;
  ~File();

  Size read(void *buf, Size sz);
  Size write(const void * buf, Size sz);

  bool flush();

  FileView map(Protect protect, const char *name = nullptr);
  FileView map(Protect protect, size_t offset, Size size, const char *name = nullptr);

private:
  void *m;
  Access m_access;
};

class FileView {
public:
  FileView(const FileView& other);
  ~FileView();

  FileView& operator=(const FileView& other) = delete;

  void *get() const;
  void flush(File::Size sz = 0);

  uint8_t& operator[](size_t offset);

private:
  friend class File;

  FileView(void *mapping, File::Access access, size_t offset, File::Size size);

  void *m;
  void *m_ptr;

  unsigned *m_ref;
};

}