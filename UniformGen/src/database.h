#pragma once

#define _CRT_SECURE_NO_WARNINGS 1

#include <string>
#include <vector>
#include <unordered_map>
#include <ctime>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using Key = std::string;
using Record = FILETIME;

static unsigned long long get_record(Record record)
{
  ULARGE_INTEGER uli;
  uli.LowPart = record.dwLowDateTime;
  uli.HighPart = record.dwHighDateTime;

  return uli.QuadPart;
}

static Record put_record(unsigned long long ull)
{
  ULARGE_INTEGER uli;
  uli.QuadPart = ull;

  Record record;
  record.dwLowDateTime = uli.LowPart;
  record.dwHighDateTime = uli.HighPart;

  return record;
}

class Database {
public:
  Database(const char *fname);

  bool compareWithRecord(const Key& key, const Record& record);
  void writeRecord(const Key& key, const Record& record);

  void serialize();

private:
  void panic(const char *reason);

  std::string m_fname;
  std::unordered_map<Key, Record> m_data;
};