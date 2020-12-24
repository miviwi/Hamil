#if !defined(NO_SSE)

template <unsigned N>
[[using gnu: always_inline]] FixedBitVector shiftBytesRight() const
{
  FixedBitVector result;

  __m128i val = _mm_load_si128((const __m128i *)bits.bytes);
  val = _mm_srli_si128(val, N);

  _mm_store_si128((__m128i *)result.bits.bytes, val);

  return result;
}

template <unsigned N>
[[using gnu: always_inline]] FixedBitVector shiftBytesLeft() const
{
  FixedBitVector result;

  __m128i val = _mm_load_si128((const __m128i *)bits.bytes);
  val = _mm_slli_si128(val, N);

  _mm_store_si128((__m128i *)result.bits.bytes, val);

  return result;
}

#else

template <unsigned N>
FixedBitVector shiftBytesRight() const
{
  return shiftRight(N*8);    // Fall-back to 64-bit operations
}

template <unsigned N>
FixedBitVector shiftBytesLeft() const
{
  return shiftLeft(N*8);    // Fall-back to 64-bit operations
}

#endif

#if !defined(NO_AVX)

// The _mm_popcnt() intrinsics are not part of AVX,
//   however all CPU's which support AVX also have
//   the ABM (AMD) or POPCNT (Intel) cpuid flag
[[using gnu: always_inline]] uint64_t popcount() const
{
  const auto lo = _mm_popcnt_u64(bits.lo);
  const auto hi = _mm_popcnt_u64(bits.hi);

  return lo + hi;
}

#else

uint64_t popcount() const
{
  auto popcount_u32 = [](uint32_t v) -> unsigned {
    static constexpr uint64_t CoeffA = 0x01001001001001ull;
    static constexpr uint64_t MaskB  = 0x84210842108421ull;

    unsigned count;

    count  = ((v&0xFFF) * CoeffA & MaskB) % 0x1F;
    count += (((v&0xFFF000) >> 12) * CoeffA & MaskB) % 0x1F;
    count += ((v >> 24) * CoeffA & MaskB) % 0x1F; 

    return count;
  };

  auto lo = bits.lo;
  auto hi = bits.hi;

  uint32_t lo_lo = (uint32_t)(lo & 0xFFFFFFFF),
           lo_hi = (uint32_t)(lo >> 32),
           hi_lo = (uint32_t)(hi & 0xFFFFFFFF),
           hi_hi = (uint32_t)(hi >> 32);

  return popcount_u32(lo_lo) + popcount_u32(lo_hi)
       + popcount_u32(hi_lo) + popcount_u32(hi_hi);
}

#endif
