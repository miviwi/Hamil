#include "database.h"

#include <cstdint>
#include <cstdio>

using byte = unsigned char;

using u16 = uint16_t;

static const byte good_header[] = {
    '0', '0', 'D', 'B',
};

Database::Database(const char *fname) :
  m_fname(fname)
{
  FILE *fp = fopen(m_fname.c_str(), "rb");

  if(!fp) {
    printf("No Database `%s' found...\n\n", fname);
    return;
  }

  byte header[4];
  for(int i = 0; i < 4; i++) {
    auto c = fgetc(fp);
    if(c == EOF) panic("inavlid database file!");

    header[i] = (byte)c;
  }

  auto header_cmp = memcmp(header, good_header, sizeof(header));
  if(header_cmp) panic("invalid adtabase file header!");

  unsigned num_records = 0;
  fread(&num_records, sizeof(num_records), 1, fp);

  for(unsigned i = 0; i < num_records; i++) {
    u16 key_length = 0;
    fread(&key_length, sizeof(key_length), 1, fp);

    std::vector<char> key_buf(key_length+1);
    fread(key_buf.data(), sizeof(char), key_length, fp);

    Key key(key_buf.data());

    Record record;
    memset(&record, 0, sizeof(record));
    fread(&record, sizeof(record), 1, fp);

    writeRecord(key, record);
  }

  fclose(fp);

  printf("Successfully loaded Database `%s'...\n", fname);
  for(auto& entry : m_data) {
    auto record = get_record(entry.second);

    printf("  `%s': %llu\n", entry.first.c_str(), record);
  }
}

bool Database::compareWithRecord(const Key& key, const Record& record)
{
  auto entry = m_data.find(key);
  if(entry == m_data.end()) return false;

  return get_record(entry->second) == get_record(record);
}

void Database::writeRecord(const Key& key, const Record& record)
{
  m_data[key] = record;
}

void Database::serialize()
{
  FILE *fp = fopen(m_fname.c_str(), "wb");

  fwrite(good_header, sizeof(byte), 4, fp);

  unsigned num_records = (unsigned)m_data.size();
  fwrite(&num_records, sizeof(num_records), 1, fp);

  for(auto& entry : m_data) {
    const Key& key = entry.first;
    const Record& record = entry.second;
    
    u16 key_length = (u16)key.length();
    fwrite(&key_length, sizeof(key_length), 1, fp);

    fwrite(key.c_str(), sizeof(char), key_length, fp);

    fwrite(&record, sizeof(record), 1, fp);
  }

  fclose(fp);
}

void Database::panic(const char *reason)
{
  char str[256];
  sprintf_s(str, "Reason: %s", reason);

  MessageBoxA(nullptr, str, "UniformGen::Database panic!", MB_OK);
  ExitProcess(-1);
}
