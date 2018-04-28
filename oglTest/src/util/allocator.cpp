#include <util/allocator.h>

#include <algorithm>

FreeListAllocator::FreeListAllocator(size_t sz) :
  m_sz(sz), m_ptr(0)
{
}

size_t FreeListAllocator::alloc(size_t sz)
{
  if(m_free_list.empty()) return allocAtEnd(sz);

  for(auto iter = m_free_list.begin(); iter != m_free_list.end(); iter++) {
    if(iter->second < sz) continue;

    auto ret = iter->first,
      leftover = iter->second-sz;

    m_free_list.erase(iter);
    if(leftover > 0) m_free_list.push_front({ ret+sz, leftover });

    return ret;
  }

  return allocAtEnd(sz);
}

void FreeListAllocator::dealloc(size_t offset, size_t sz)
{
  if(m_ptr == offset+sz) {
    m_ptr -= sz;
    return;
  }

  m_free_list.push_front({ offset, sz });
  coalesce();
}

size_t FreeListAllocator::allocAtEnd(size_t sz)
{
  if(m_ptr+sz >= m_sz) return Error;

  auto ret = m_ptr;
  m_ptr += sz;

  return ret;
}

void FreeListAllocator::coalesce()
{
  auto in_range = [](Range a, Range b) -> bool
  {
    if(a.first < b.first) return a.first+a.second >= b.first;
    else                  return b.first+b.second >= a.first;

    return false;
  };

  auto prev = m_free_list.begin(),
    iter = ++m_free_list.begin();
  while(iter != m_free_list.end()) {
    if(in_range(*prev, *iter)) {
      Range a = { prev->first, prev->first+prev->second },
        b = { iter->first, iter->first+iter->second };

      iter->first = std::min(a.first, b.first);
      iter->second = std::max(a.second, b.second) - iter->first;

      m_free_list.erase(prev);
    }
    prev = iter;
    iter++;
  }
}
