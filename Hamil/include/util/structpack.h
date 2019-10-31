#pragma once

#if defined(_MSVC_VER)
#  define PACKED_STRUCT_BEGIN   __pragma( pack(push, 1) )
#  define PACKED_STRUCT_END     __pragma( pack(pop) )
#else
#  define PACKED_STRUCT_BEGIN
#  define PACKED_STRUCT_END     __attribute__((__packed__))
#endif
