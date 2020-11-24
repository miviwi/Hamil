#include <sysv/cpuinfo.h>
#include <os/cpuinfo.h>

#include <config>

#include <algorithm>
#include <iterator>
#include <sstream>
#include <unordered_map>

#include <cassert>
#include <cstddef>
#include <cctype>
#include <cstdio>

namespace sysv {

os::CpuInfo *create_cpuinfo()
{
  auto self = new os::CpuInfo();

  auto cpuinfo = cpuinfo_detail::cpuinfo();

  self->m_num_logical_processors  = cpuinfo->num_threads;
  self->m_num_physical_processors = cpuinfo->num_cores;

  return self;
}

namespace cpuinfo_detail {

bool ProcCPUInfo::hyperthreading() const
{
  return num_threads > num_cores;
}

size_t ProcCPUInfo::numFlags() const
{
  return flags_vector.size();
}

std::string ProcCPUInfo::flag(size_t idx) const
{
  const auto& flag_str_view = flags_vector.at(idx);
  return std::string(flag_str_view.data(), flag_str_view.size());
}

bool ProcCPUInfo::hasFlag(const std::string& flag) const
{
  return flags_set.find(flag) != flags_set.cend();
}

std::unique_ptr<ProcCPUInfo> ProcCPUInfo::parse_proc_cpuinfo(const std::string& cpuinfo)
{
  auto cpuinfo_ptr = new ProcCPUInfo();
  auto& self = *cpuinfo_ptr;

  std::unordered_map<std::string, std::string> fields;

  std::istringstream ss(cpuinfo);
  std::string line;
  while(std::getline(ss, line, '\n')) {
    bool all_whitespace = std::all_of(line.begin(), line.end(),
        [](char ch) { return isspace(ch); });

    // Once an empty line has been found - we've reached
    //   the end of the data for a single logical processor
    if(line.empty() || all_whitespace) break;

    auto colon_pos = line.find(':');

    // Trim any excess whitespace on the name and value

    auto field_name_view  = std::string_view(line.data(), colon_pos);
    auto field_value_view = std::string_view(line.data() + colon_pos+1);

    auto field_name_end_it = std::find_if_not(field_name_view.rbegin(), field_name_view.rend(),
        [](char ch) { return isspace(ch); });
    auto field_name_trim_pos = (size_t)std::distance(field_name_view.rbegin(), field_name_end_it);

    auto field_value_trim_pos = field_value_view.find_first_not_of(" \t");

    if(field_name_trim_pos != std::string_view::npos)  field_name_view.remove_suffix(field_name_trim_pos);
    if(field_value_trim_pos != std::string_view::npos) field_value_view.remove_prefix(field_value_trim_pos);

    // Add the field's (name, value) pair to the std::unordered_map
    fields.emplace(
        std::string(field_name_view.data(), field_name_view.size()),
        std::string(field_value_view.data(), field_value_view.size())
    );
  }

  auto num_threads_field_it = fields.find("siblings");
  auto num_cores_field_it   = fields.find("cpu cores");

  auto vendor_field_it = fields.find("vendor_id");

  auto flags_field_it = fields.find("flags");

  // Sanity check
  assert(
         num_threads_field_it != fields.end()
      && num_cores_field_it   != fields.end()
      && vendor_field_it      != fields.end()
      && flags_field_it       != fields.end()
  );

  const auto& num_threads_str = num_threads_field_it->second;
  const auto& num_cores_str   = num_cores_field_it->second;

  int num_threads = (unsigned)std::stoi(num_threads_str);
  int num_cores   = (unsigned)std::stoi(num_cores_str);

  // Another sanity check
  assert(num_threads > 0 && num_cores > 0);

  self.num_threads = (unsigned)num_threads;
  self.num_cores   = (unsigned)num_cores;

  self.vendor = std::move(vendor_field_it->second);

  // Order of operations is important here, so
  //   the std::string_views stored in:
  //     - self.flags_vector, self.flags_set
  //   won't point to invalid memory
  //   i.e. move first, THEN take the reference
  self.flags_ = std::move(flags_field_it->second);

  const auto& flags = self.flags_;

  size_t prev_flag_pos = 0,
         next_flag_pos = flags.find(' ');
  while(next_flag_pos != std::string::npos) {
    auto offset = prev_flag_pos;
    auto length = next_flag_pos - prev_flag_pos;

    std::string_view flag(flags.data() + offset, length);

    self.flags_vector.push_back(flag);
    self.flags_set.insert(flag);

    prev_flag_pos = next_flag_pos+1;
    next_flag_pos = flags.find(' ', prev_flag_pos);
  }

  return std::unique_ptr<ProcCPUInfo>(cpuinfo_ptr);
}

}

}
