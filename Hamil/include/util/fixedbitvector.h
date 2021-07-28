#pragma once

#include <util/wideint.h>
#include <util/simd.h>

#include <type_traits>
#include <limits>
#include <array>
#include <algorithm>
#include <string>

#include <cassert>
#include <cstddef>
#include <climits>

namespace util {

namespace detail {

template <typename T>
struct is_1d_array {
  static constexpr bool value = std::is_array_v<T> && (std::rank_v<T> == 1);
};

template <typename T, size_t N>
struct is_1d_array<std::array<T, N>> : std::true_type { };

template <typename T>
inline constexpr bool is_1d_array_v = is_1d_array<T>::value;

}

template <unsigned Width>
struct FixedBitVector {
  constexpr FixedBitVector()
  {
    assert(0 && "given FixedBitVector Width is not allowed!");
  }
};

template <>
struct alignas(sizeof(u128)) FixedBitVector<128> {

  // Initialized the FixedBitVector with data stored
  //  at *p or by calling std::array::data()
  //    - The bytes/words used to initialize the vector
  //      are expected to come in little-endian ordering
  template <typename PtrOrArray>
  constexpr FixedBitVector(const PtrOrArray& p) {
    static_assert(
        std::is_pointer_v<PtrOrArray> || detail::is_1d_array_v<PtrOrArray>,
        "ctor argument must be eiter a pointer or an array reference!"
    );


    if constexpr(std::is_pointer_v<PtrOrArray>) {
      const auto value = (const uint8_t *)p;
      std::copy(value, value+sizeof(u128), bits.bytes);
    } else if constexpr(std::is_array_v<PtrOrArray>) {   // Doesn't match std::array
      static_assert(sizeof(PtrOrArray) == sizeof(u128));

      const auto value = (const uint8_t *)p;
      std::copy(value, value+sizeof(u128), bits.bytes);
    } else {
      assert(sizeof(typename PtrOrArray::value_type)*p.size() == sizeof(u128));

      const auto value = (uint8_t *)p.data();
      std::copy(value, value+sizeof(u128), bits.bytes);
    }

  }

  FixedBitVector() { }

  // Returns a zero-initialized FixedBitVector
  static FixedBitVector zero()
  {
    return FixedBitVector(std::array<uint64_t, 2>{ 0u, 0u });
  }

  static FixedBitVector one()
  {
    return FixedBitVector(std::array<uint64_t, 2>{ 1u, 0u /* little-endian */});
  }

  // Returns a FixedBitVector with 'n' lowest (counted
  //   from LSB->MSB) bits set and the rest cleared, ex.
  //       mask_n_lsbs(5)  == 0x000000000000001f
  //       mask_n_lsbs(54) == 0x003fffffffffffff
  //       mask_n_lsbs(0)  == 0x0000000000000000
  static FixedBitVector mask_n_lsbs(unsigned n)
  {
    static constexpr auto Fullu64Mask = std::numeric_limits<uint64_t>::max();  // 64 1-bits

    // Determine how many bits of the lo/hi parts must be set...
    uint64_t n_lo_bits = n % 64,
             n_hi_bits = (uint64_t)std::max((int64_t)n - 64l, 0l);

    //  ...convert to bitmasks...
    uint64_t lo_mask = (1ull<<n_lo_bits) - 1ull,
             hi_mask = (1ull<<n_hi_bits) - 1ull;

    //  ...detect when the lo and/or hi part(s) should have
    //     all their bits set (as lo_mask/hi_mask above would
    //     be == 0 instead of the desired ~0 due to an overflow)
    bool lo_mask_valid = n < 64,
         hi_mask_valid = n < 128;

    return FixedBitVector(std::array<uint64_t, 2>{
        lo_mask_valid ? lo_mask : Fullu64Mask,   // Little-endian: LSB-first
        hi_mask_valid ? hi_mask : Fullu64Mask,   //             ...MSB-last
    });
  }

  FixedBitVector clone() const { return FixedBitVector(*this); }

  // Sets/clears the bit at position 'idx' starting at 0
  //   and moving from LSB -> MSB
  FixedBitVector set(unsigned idx) const;
  FixedBitVector clear(unsigned idx) const;

  // Returns 'true' if the bit at idx'th position is set
  bool test(unsigned idx) const;

  bool equal(const FixedBitVector& other) const;

  // Returns the result of the comparison
  //     - this > 0
  bool gtZero() const;

  // Returns a new FixedBitVector which is the bitwise AND
  //   of this and other
  FixedBitVector bitAnd(const FixedBitVector& other) const;

  // Returns a new FixedBitVector which is the bitwise OR
  //   of this and other
  FixedBitVector bitOr(const FixedBitVector& other) const;

  // Returns a new FixedBitVector which is a bitwise
  //   negation (NOT) of this vector
  FixedBitVector bitNot() const;

  // Returns a new FixedBitVector shifted right logically
  //   i.e. without retaining the sign, by 'amount' bits
  FixedBitVector shiftRight(unsigned amount) const;
  // Returns a new FixedBitVector shifted left
  FixedBitVector shiftLeft(unsigned amount)  const;

  // Sets idx'th bit of the vector inplace
  FixedBitVector& setMut(unsigned idx);
  // Clears idx'th bit of the vector inplace
  FixedBitVector& clearMut(unsigned idx);

  // Includes SSE/AVX implementations of certain methods
  #include <util/fixedbitvector.hh>

  /***************************************************************
   * Declared and defined according to CPU feature compile
       configuration flags NO_SSE/NO_AVX in <fixedbitvector.hh>
       above

  FixedBitVector shiftBytesRight() const;
  FixedBitVector shiftBytesLeft() const;

  uint64_t popcount() const;

  ****************************************************************/

  u128 bits;

private:
};

template<> struct FixedBitVector<128>;

template <unsigned Width>
inline bool operator==(const FixedBitVector<Width>& a, const FixedBitVector<Width>& b)
{
  return a.template equal(b);
}

template <unsigned Width>
inline bool operator!=(const FixedBitVector<Width>& a, const FixedBitVector<Width>& b)
{
  return !(a.template equal(b));
}

using BitVector128 = FixedBitVector<128>;

enum BitVectorStringifyType {
  StringifyHex,
  StringifyBinary,
};

std::string to_str(const BitVector128& v, BitVectorStringifyType type = StringifyHex);

}
