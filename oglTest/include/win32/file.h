#pragma once

#include <common.h>

#include <util/ref.h>
#include <util/bit.h>

#include <functional>

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

    ReadWrite = Read|Write,
  };

  enum Share {
    ShareNone  = 0,
    ShareRead  = (1<<0),
    ShareWrite = (1<<1),
  };

  enum OpenMode {
    CreateAlways, // Create new, overwrite existing
    CreateNew,    // Create new, fail otherwise
    OpenAlways,   // Open file, append existing
    OpenExisting, // Open file, fail otherwise
    TruncateExisting, // Opens a file and truncates it to 0 bytes,
                      // fails if it doesn't exist
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

  enum Seek {
    SeekBegin,
    SeekCurrent,
    SeekEnd,
  };

  struct Error {
    const unsigned what;
    Error(unsigned what_) : what(what_) { }

    const char *errorString() const;
  };

  struct FileOpenError : public Error {
    using Error::Error;
  };
  struct MappingCreateError : public Error {
    using Error::Error;
  };
  struct MapFileError : public Error {
    using Error::Error;
  };
  struct GetFileInfoError : public Error {
    using Error::Error;
  };

  File(const char *path, Access access, Share share, OpenMode open);
  File(const char *path, Access access, OpenMode open);
  File(const char *path, Access access);
  File(const File& other) = delete;
  ~File();

  size_t size() const;

  const char *fullPath() const;

  Size read(void *buf, Size sz);
  // Reads all of the File
  Size read(void *buf); 
  Size write(const void *buf, Size sz);

  void seek(Seek seek, long offset) const;

  bool flush();

  FileView map(Protect protect, const char *name = nullptr);
  FileView map(Protect protect, size_t offset, size_t size, const char *name = nullptr);

private:
  void *m;
  Access m_access;
  size_t m_sz;

  mutable char *m_full_path; // lazy-initialized
};

class FileView : public Ref {
public:
  ~FileView();

  FileView& operator=(const FileView& other) = delete;

  void *get() const;
  template <typename T> T *get() const { return (T *)get(); }

  void flush(File::Size sz = 0);

  uint8_t& operator[](size_t offset);

private:
  friend class File;

  FileView(void *mapping, File::Protect access, size_t offset, size_t size);

  void *m;
  void *m_ptr;
};

class FileQuery {
public:
  enum Attributes : unsigned {
    IsDirectory = 1<<0,
    IsFile      = 1<<1, 
  };

  using IterFn = std::function<void(const char *name, Attributes attrs)>;

  struct Error { };

  FileQuery();
  FileQuery(const char *path);
  FileQuery(const FileQuery& other) = delete;
  FileQuery(FileQuery&& other);
  ~FileQuery();

  FileQuery& operator=(const FileQuery& other) = delete;
  FileQuery& operator=(FileQuery&& other);

  void foreach(IterFn fn);

private:
  void openQuery(const char *path);
  void closeQuery();

  void *m;
  void /* WIN32_FIND_DATAA */ *m_find_data;
};

bool current_working_directory(const char *dir);

}