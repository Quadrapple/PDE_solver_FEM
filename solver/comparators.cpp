#include "comparators.h"

int less_msb(uint64_t a, uint64_t b) { return a < b && a < (a ^ b); }

bool compareZorder(const glm::dvec2 &p1, const glm::dvec2 &p2) {
  glm::ivec2 p1Exp;
  glm::ivec2 p2Exp;

  double p1xNormed = frexp(p1.x, &p1Exp.x);
  double p1yNormed = frexp(p1.y, &p1Exp.y);

  double p2xNormed = frexp(p2.x, &p2Exp.x);
  double p2yNormed = frexp(p2.y, &p2Exp.y);

  glm::ivec2 expDiff = {glm::max(p1Exp.x, p2Exp.x), glm::max(p1Exp.y, p2Exp.y)};

  if (expDiff.x > expDiff.y) {
    return p1.x < p2.x;
  } else if (expDiff.x < expDiff.y) {
    return p1.y < p2.y;
  }

  glm::u64vec2 p1Mantissa = {std::bit_cast<uint64_t>(p1xNormed),
                             std::bit_cast<uint64_t>(p1yNormed)};
  glm::u64vec2 p2Mantissa = {std::bit_cast<uint64_t>(p2xNormed),
                             std::bit_cast<uint64_t>(p2yNormed)};
  ;

  if (less_msb(p1Mantissa.x ^ p2Mantissa.x, p1Mantissa.y ^ p2Mantissa.y)) {
    return p1.y < p2.y;
  }
  return p1.x < p2.x;
}
