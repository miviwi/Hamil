#include <util/hashindex.h>

#include <cassert>
#include <cstring>

#include <new>
#include <algorithm>

namespace util {

HashIndex::Index HashIndex::InvalidIndex[1] ={
  HashIndex::Invalid,
};

HashIndex::HashIndex()
{
  init(InitialHashSize, InitialHashSize);
}

HashIndex::HashIndex(size_t hash_sz, size_t chain_sz)
{
  init(hash_sz, chain_sz);
}

HashIndex::HashIndex(const HashIndex& other) :
  m_granularity(other.m_granularity),
  m_hash_mask(other.m_hash_mask), m_lookup_mask(other.m_lookup_mask)
{
  if(other.m_lookup_mask == NoLookup) {
    m_hash_sz  = other.m_hash_sz;
    m_chain_sz = other.m_chain_sz;

    dealloc();
    return;
  }

  if(other.m_hash_sz != m_hash_sz || m_hash == InvalidIndex) {
    if(m_hash != InvalidIndex) delete[] m_hash;

    m_hash_sz = other.m_hash_sz;
    m_hash    = new Index[m_hash_sz];
  }

  if(other.m_chain_sz != m_chain_sz || m_chain == InvalidIndex) {
    if(m_chain != InvalidIndex) delete[] m_chain;

    m_chain_sz = other.m_chain_sz;
    m_chain    = new Index[m_chain_sz];
  }

  memcpy(m_hash, other.m_hash, other.m_hash_sz * sizeof(Index));
  memcpy(m_chain, other.m_chain, other.m_chain_sz * sizeof(Index));
}

HashIndex::HashIndex(HashIndex&& other) :
  m_hash_sz(other.m_hash_sz), m_hash(other.m_hash),
  m_chain_sz(other.m_chain_sz), m_chain(other.m_chain),
  m_granularity(other.m_granularity),
  m_hash_mask(other.m_hash_mask), m_lookup_mask(other.m_lookup_mask)
{
  other.init(InitialHashSize, InitialHashSize);
}

HashIndex::~HashIndex()
{
  dealloc();
}

HashIndex& HashIndex::operator=(const HashIndex& other)
{
  dealloc();

  new(this) HashIndex(other);
  return *this;
}

HashIndex& HashIndex::operator=(HashIndex&& other)
{
  dealloc();

  new(this) HashIndex(other);
  return *this;
}

void HashIndex::add(Key key, Index idx)
{
  if(m_hash == InvalidIndex) {
    alloc(m_hash_sz, idx >= m_chain_sz ? idx+1 : m_chain_sz);
  } else if(idx > m_chain_sz) {
    resize(idx+1);
  }

  auto h = key & m_hash_mask;

  m_chain[idx] = m_hash[h];
  m_hash[h] = idx;
}

void HashIndex::remove(Key key, Index idx)
{
  auto k = key & m_hash_mask;

  if(m_hash == InvalidIndex) return;

  if(m_hash[k] == idx) {
    m_hash[k] = m_chain[idx];
  } else {
    for(auto i = m_hash[k]; i != Invalid; i = m_chain[i]) {
      if(m_chain[i] != idx) continue;

      m_chain[i] = m_chain[idx];
      break;
    }
  }

  m_chain[idx] = Invalid;
}

HashIndex::Index HashIndex::first(Key key) const
{
  return m_hash[key & m_hash_mask & m_lookup_mask];
}

HashIndex::Index HashIndex::next(Index idx) const
{
  assert(idx < m_chain_sz && "Invalid index for HashIndex::next()!");

  return m_chain[idx & m_lookup_mask];
}

size_t HashIndex::hashSize() const
{
  return m_hash_sz;
}

size_t HashIndex::chainSize() const
{
  return m_chain_sz;
}

unsigned HashIndex::spread() const
{
  if(m_hash == InvalidIndex) return 100;

  int total_items = 0;
  std::vector<unsigned> num_hash_items(m_hash_sz);
  for(size_t i = 0; i < num_hash_items.size(); i++) {
    for(Index idx = m_hash[i]; idx >= 0; idx = m_chain[idx]) {
      num_hash_items[i]++;
    }

    total_items += num_hash_items[i];
  }

  if(total_items <= 1) return 100;

  int avg = total_items / (int)m_hash_sz;
  int err = 0;
  for(auto ni : num_hash_items) {
    int e = abs((int)ni - avg);

    if(e > 1) err += e-1;
  }

  return 100 - (err*100 / total_items);
}

void HashIndex::init(size_t hash_sz, size_t chain_sz)
{
  assert((hash_sz & (hash_sz-1)) == 0 && "HashIndex::hash_sz must be a power of 2!");

  m_hash_sz  = hash_sz;
  m_chain_sz = chain_sz;

  m_hash = m_chain = InvalidIndex;

  m_granularity = InitialHashGranularity;

  m_hash_mask = (Key)(m_hash_sz - 1);
  m_lookup_mask = NoLookup;
}

void HashIndex::alloc(size_t hash_sz, size_t chain_sz)
{
  assert((hash_sz & (hash_sz-1)) == 0 && "HashIndex::hash_sz must be a power of 2!");

  dealloc();

  m_hash_sz  = hash_sz;
  m_chain_sz = chain_sz;

  m_hash  = new Index[hash_sz];
  m_chain = new Index[chain_sz];

  std::fill(m_hash, m_hash+m_hash_sz, Invalid);
  std::fill(m_chain, m_chain+m_chain_sz, Invalid);

  m_hash_mask   = (Key)(m_hash_sz - 1);
  m_lookup_mask = DoLookup;
}

void HashIndex::resize(size_t chain_sz)
{
  if(chain_sz <= m_chain_sz) return;

  auto mod = chain_sz % m_granularity;
  auto new_size = mod ? (chain_sz + m_granularity - mod) /* round to m_granularity */ : chain_sz;

  if(m_chain == InvalidIndex) {
    m_chain_sz = new_size;
    return;
  }

  auto old_chain = m_chain;
  m_chain = new Index[new_size];

  memcpy(m_chain, old_chain, m_chain_sz*sizeof(Index));
  std::fill(m_chain+m_chain_sz, m_chain+new_size, Invalid);

  m_chain_sz = new_size;
}

void HashIndex::clear()
{
  std::fill(m_hash, m_hash+m_hash_sz, Invalid);
}

void HashIndex::dealloc()
{
  if(m_hash != InvalidIndex) {
    delete[] m_hash;
    m_hash = InvalidIndex;
  }

  if(m_chain != InvalidIndex) {
    delete[] m_chain;
    m_chain = InvalidIndex;
  }

  m_lookup_mask = NoLookup;
}

}
