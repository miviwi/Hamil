#pragma once

#if !defined(NO_SSE)
#  include <xmmintrin.h>
#  include <pmmintrin.h>
#  include <emmintrin.h>
#endif

#if !defined(NO_AVX)
#  include <smmintrin.h>
#  include <immintrin.h>
#  include <nmmintrin.h>
#endif
