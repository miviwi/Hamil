#include <math/compare.h>

#include <cmath>

namespace math {

bool equal(float a, float b)
{
  return abs(a-b) <= EPSILON;
}

}