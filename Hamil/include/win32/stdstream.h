#pragma once

#include <common.h>

namespace win32::stdstream_detail {

void init();
void finalize();

int do_read(void *buf, size_t buf_sz);

}
