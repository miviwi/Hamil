#pragma once

#include <common.h>

#include <util/polystorage.h>
#include <os/error.h>
#include <os/waitable.h>

#include <cstddef>

#include <memory>
#include <functional>

namespace os {

// Forward declaration
class FileView;

// PIMPL struct
struct FileData;

class File : public Ref {
public:
  using Size = unsigned;

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
    ProtectPrivate = (1<<3), // makes the mapping copy-on-write
                             // can be combined only with ProtectExecute

    ProtectReadWrite        = ProtectRead|ProtectWrite,
    ProtectExecuteRead      = ProtectExecute|ProtectRead,
    ProtectExecutePrivate   = ProtectExecute|ProtectPrivate,
    ProtectExecuteReadWrite = ProtectExecute|ProtectRead|ProtectWrite,
  };

  enum Seek {
    SeekBegin,
    SeekCurrent,
    SeekEnd,
  };

  enum : size_t {
    SeekOffsetInvalid = (size_t)~0u,

    ReadWriteFailed = (size_t)~0u,
  };

  struct FileOpenError : public os::Error {
    using Error::Error;
  };
  struct MappingCreateError : public os::Error {
    using Error::Error;
  };
  struct MapFileError : public os::Error {
    using Error::Error;
  };
  struct GetFileInfoError : public os::Error {
    using Error::Error;
  };

  using Ptr = std::unique_ptr<File>;

  static Ptr create();

  virtual ~File();

  File& open(
      const char *path, Access access,
      Share share = ShareRead, OpenMode mode = OpenAlways
  );

  // Returns 'true' is this File has previously had open() called on it
  bool isOpen() const;
  operator bool() const { return isOpen(); }

  virtual size_t size() const = 0;

  virtual const char *fullPath() const = 0;

  virtual size_t read(void *buf, size_t sz) = 0;
  virtual size_t read(void *buf) = 0;    // Reads the whole file
  virtual size_t write(const void *buf, size_t sz) = 0;

  virtual File& seek(Seek whence, size_t offset) = 0;
  virtual size_t seekOffset() const = 0;

  virtual bool flush() = 0;

  std::unique_ptr<FileView> map(Protect prot, size_t offset, size_t size, const char *name = nullptr);
  std::unique_ptr<FileView> map(Protect prot, const char *name = nullptr);

protected:
  File(size_t storage_sz);

  virtual void doOpen(
      const char *path, Access access, Share share, OpenMode mode
  ) = 0;

  void *storage();
  const void *storage() const;

  template <typename T>
  T *storage()
  {
    return (T *)storage();
  }
  template <typename T>
  const T *storage() const
  {
    return (const T *)storage();
  }

private:
  friend FileView;

  bool m_open;

  FileData *m_data;
};

class FileView {
public:
  using Size = File::Size;
  using Protect = File::Protect;

  using Ptr = std::unique_ptr<FileView>;

  virtual ~FileView();

  void *get() const;
  template <typename T> T *get() const { return (T *)get(); }
  u8& operator[](size_t offset);

  size_t size() const;

  void flush(File::Size size = 0);

  virtual void unmap() = 0;

protected:
  FileView(File *file, size_t size, const char *name);

  File *origin() const;

  virtual void *doMap(Protect prot, size_t offset) = 0;
  virtual void doFlush(Size size) = 0;

  void *m_ptr;

private:
  friend File;

  File *m_file;
  size_t m_size;
};

class FileQuery {
public:
  static void deleter(FileQuery *q);

  enum Attributes : unsigned {
    IsDirectory = 1<<0,
    IsFile      = 1<<1, 
  };

  using IterFn = std::function<void(const char *name, Attributes attrs)>;

  struct Error { };

  using Ptr = std::unique_ptr<FileQuery, decltype(&FileQuery::deleter)>;

  static Ptr null();
  static Ptr open(const char *path);

  FileQuery() = default;
  FileQuery(const FileQuery& other) = delete;
  virtual ~FileQuery() = default;

  FileQuery& operator=(const FileQuery& other) = delete;
  FileQuery& operator=(FileQuery&& other);

  virtual void foreach(IterFn fn) = 0;

protected:
  virtual void doOpen(const char *path) = 0;

};

bool current_working_directory(const char *dir);

}
