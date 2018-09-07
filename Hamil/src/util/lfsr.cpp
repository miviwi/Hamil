#include <util/lfsr.h>

namespace util {

MaxLength32BitLFSR::MaxLength32BitLFSR(u32 seed) :
  m(seed)
{
}

u32 MaxLength32BitLFSR::next()
{
  auto val = m;
  auto lsb = m & 1;

  m >>= 1;
  if(lsb) m ^= Feedback;

  return val;
}

}