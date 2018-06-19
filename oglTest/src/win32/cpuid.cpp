#include <win32/cpuid.h>
#include <win32/panic.h>

#include <Windows.h>

namespace win32 {

void check_sse_sse2_support()
{
  auto sse  = IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE);
  auto sse2 = IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE);
  
  if(sse && sse2) return;

  panic("Your CPU doesn't support the SSE2 instruction set which is required.", -1);
}

}