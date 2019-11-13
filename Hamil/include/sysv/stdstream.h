#pragma once

#include <common.h>

namespace sysv::stdstream_detail {

void init();
void finalize();

int do_read(void *buf, size_t buf_sz);

}
