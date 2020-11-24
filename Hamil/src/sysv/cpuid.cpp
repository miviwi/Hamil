#include <sysv/cpuid.h>
#include <sysv/cpuinfo.h>

#include <config>

#include <cassert>
#include <cstring>

namespace sysv {

os::CpuId cpuid()
{
  os::CpuId self;

  auto cpuinfo = cpuinfo_detail::cpuinfo();

  assert(cpuinfo && "os::cpuid() called before os::init()!");

  memset(&self, 0, sizeof(self));
  memcpy(self.vendor, cpuinfo->vendor.data(), cpuinfo->vendor.size());

  // 'rdtsc' is only useful if it's constant rate and is guaranteed
  //   to always be ticking (since the code utilizing it will be
  //   measuring time - possibly between clock frequency/C state shifts)
  self.rdtsc = cpuinfo->hasFlag("tsc") && cpuinfo->hasFlag("constant_tsc")
      && cpuinfo->hasFlag("nonstop_tsc");

  self.sse   = cpuinfo->hasFlag("sse");
  self.sse2  = cpuinfo->hasFlag("sse2");
  self.sse3  = cpuinfo->hasFlag("pni");
  self.ssse3 = cpuinfo->hasFlag("ssse3");
  self.sse41 = cpuinfo->hasFlag("sse4_1");
  self.sse42 = cpuinfo->hasFlag("sse4_2");
  self.avx   = cpuinfo->hasFlag("avx");
  self.fma   = cpuinfo->hasFlag("fma");

  self.popcnt = cpuinfo->hasFlag("popcnt");
  self.f16c   = cpuinfo->hasFlag("f16c");

  return self;
}

}
