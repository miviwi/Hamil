#include <util/fixedbitvector.h>
#include <util/format.h>

#include <cstdlib>

namespace util {

FixedBitVector<128> FixedBitVector<128>::set(unsigned idx) const
{
  const auto i = std::div(idx, CHAR_BIT);

  auto result = clone();
  result.bits.bytes[i.quot] |= 1<<i.rem;

  return result;
}

FixedBitVector<128> FixedBitVector<128>::clear(unsigned idx) const
{
  const auto i = std::div(idx, CHAR_BIT);

  auto result = clone();
  result.bits.bytes[i.quot] &= ~(1<<i.rem);

  return result;
}

bool FixedBitVector<128>::test(unsigned idx) const
{
  const auto i = std::div(idx, CHAR_BIT);

  return bits.bytes[i.quot] & (1<<i.rem);
}

bool FixedBitVector<128>::equal(const FixedBitVector<128>& other) const
{
  const bool hi_eq = (bits.hi == other.bits.hi),
        lo_eq = (bits.lo == other.bits.lo);

  return hi_eq & lo_eq;    // Use bitwise AND to avoid generating Jcc (jump conditional)
}

bool FixedBitVector<128>::gtZero() const
{
  return (bits.hi | bits.lo) > 0;
}

FixedBitVector<128> FixedBitVector<128>::bitAnd(const FixedBitVector<128>& other) const
{
  auto result = clone();

  result.bits.hi &= other.bits.hi;
  result.bits.lo &= other.bits.lo;

  return result;
}

FixedBitVector<128> FixedBitVector<128>::bitOr(const FixedBitVector<128>& other) const
{
  auto result = clone();

  result.bits.hi |= other.bits.hi;
  result.bits.lo |= other.bits.lo;

  return result;
}

FixedBitVector<128> FixedBitVector<128>::bitNot() const
{
  auto result = clone();

  result.bits.hi = ~result.bits.hi;
  result.bits.lo = ~result.bits.lo;

  return result;
}

//#if defined(NO_SSE) && defined(NO_AVX)

FixedBitVector<128> FixedBitVector<128>::shiftRight(unsigned amount) const
{
  auto result = clone();

  if(amount < 64) {
    const uint64_t mask = (1<<amount) - 1ul;
    const uint64_t carry = result.bits.hi & mask;

    result.bits.hi >>= amount;
    result.bits.lo >>= amount;
    result.bits.lo |= carry << (64ul - amount);
  } else {
    result.bits.lo = result.bits.hi;
    result.bits.hi = 0;
    result.bits.lo >>= amount - 64ul;
  }

  return result;
}

FixedBitVector<128> FixedBitVector<128>::shiftLeft(unsigned amount) const
{
  auto result = clone();

  if(amount < 64) {
    const uint64_t mask = (1<<(64ul - amount)) - 1ul;
    const uint64_t carry = result.bits.lo & mask;

    result.bits.lo <<= amount;
    result.bits.hi <<= amount;
    result.bits.hi |= carry >> (64ul - amount);
  } else {
    result.bits.hi = result.bits.lo;
    result.bits.lo = 0;
    result.bits.hi <<= amount - 64ul;
  }

  return result;
}

//#endif

FixedBitVector<128>& FixedBitVector<128>::setMut(unsigned idx)
{
  return (*this = set(idx));
}

FixedBitVector<128>& FixedBitVector<128>::clearMut(unsigned idx)
{
  return (*this = clear(idx));
}

std::string to_str(const FixedBitVector<128>& v, BitVectorStringifyType type)
{
  switch(type) {
  case StringifyHex:
    return util::fmt("0x%.16llx'%.16llx",
        (unsigned long long)v.bits.hi,
        (unsigned long long)v.bits.lo
    );

  case StringifyBinary:
    assert(0 && "unimplemented");
    break;

  default: assert(0 && "invalid value for BitVectorStringifyType!");
  }

  return "";
}

}
