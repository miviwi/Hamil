#include <math/compare.h>

#include <limits>
#include <cmath>

namespace math {

bool equal(float a, float b)
{
  return abs(a-b) <= std::numeric_limits<float>::epsilon();
}

}