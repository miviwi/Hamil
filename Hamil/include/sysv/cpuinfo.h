#pragma once

#include <sysv/sysv.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <set>

namespace os {
// Forward declarations
class CpuInfo;
struct CpuId;
}

namespace sysv {
os::CpuInfo *create_cpuinfo();

// Forward declarations
void init_cpuinfo();
os::CpuId cpuid();

namespace cpuinfo_detail {

class ProcCPUInfo {
protected:
  friend void sysv::init();
  friend void sysv::init_cpuinfo();
  friend os::CpuInfo *sysv::create_cpuinfo();

  friend os::CpuId sysv::cpuid();

  static std::unique_ptr<ProcCPUInfo> parse_proc_cpuinfo(const std::string& cpuinfo);

  bool hyperthreading() const;

  size_t numFlags() const;
  std::string flag(size_t idx) const;
  bool hasFlag(const std::string& flag) const;

  unsigned num_threads;
  unsigned num_cores;

  std::string vendor;

private:
  ProcCPUInfo() = default;

  std::string flags_;

  std::vector<std::string_view> flags_vector;
  std::set<std::string_view> flags_set;
};

}

}
