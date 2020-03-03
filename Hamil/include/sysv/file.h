#pragma once

#include "util/polystorage.h"
#include <common.h>

#include <os/file.h>

namespace sysv {

// PIMPL structs
struct FileData;
struct FileQueryData;

class File final : public os::File {
public:
  File();
  virtual ~File();

  virtual size_t size() const final;

  virtual const char *fullPath() const final;

  virtual size_t read(void *buf, size_t sz) final;
  virtual size_t read(void *buf) final;    // Reads the whole file
  virtual size_t write(const void *buf, size_t sz) final;

  virtual os::File& seek(Seek whence, size_t offset) final;
  virtual size_t seekOffset() const final;

  virtual bool flush() final;

protected:
  virtual void doOpen(
      const char *path, Access access, Share share, OpenMode mode
  ) final;

private:
  friend class FileView;

  void stat() const;

  FileData& data();
  const FileData& data() const;
};

class FileView final : public os::FileView {
public:
  virtual ~FileView();

  virtual void unmap() final;

protected:
  FileView(os::File *file, size_t size, const char *name);

  virtual void *doMap(Protect prot, size_t offset) final;
  virtual void doFlush(Size size) final;

private:
  friend os::File;

};

class FileQuery final : public WithPolymorphicStorage<os::FileQuery> {
public:
  static FileQuery *alloc();
  static void destroy(FileQuery *q);

  FileQuery();
  virtual ~FileQuery();

  virtual void foreach(IterFn fn) final;

protected:
  virtual void doOpen(const char *path) final;

private:
  FileQueryData& data();
};

bool current_working_directory(const char *dir);

}
