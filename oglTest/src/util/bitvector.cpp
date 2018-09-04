#include <util/bitvector.h>

#include <cassert>

namespace util {

BitVector::BitVector(size_t alloc) :
  m_sz(0)
{
  m.reserve(alloc / ElemBits);
}

bool BitVector::get(size_t idx) const
{
  auto elem_idx = idx >> ElemShift;
  auto bit_idx  = idx & ElemMask;

  auto elem = m[elem_idx];

  return (elem >> bit_idx) & 1;
}

void BitVector::set(size_t idx)
{
  auto elem_idx = idx >> ElemShift;
  auto bit_idx  = idx & ElemMask;

  auto elem = m[elem_idx];

  elem |= (1ull << bit_idx);

  m[elem_idx] = elem;
}

void BitVector::clear(size_t idx)
{
  auto elem_idx = idx >> ElemShift;
  auto bit_idx  = idx & ElemMask;

  auto elem = m[elem_idx];

  elem &= ~(1 << bit_idx);

  m[elem_idx] = elem;
}

void BitVector::put(size_t idx, bool val)
{
  if(idx >= size()) {
    resize(idx+1);
  }

  val ? set(idx) : clear(idx);
}

size_t BitVector::append(bool val)
{
  auto idx = size();

  put(idx, val);

  return idx;
}

void BitVector::resize(size_t new_size)
{
  m_sz = new_size;
  m.resize(new_size / ElemBits);
}

size_t BitVector::size() const
{
  return m_sz;
}

}