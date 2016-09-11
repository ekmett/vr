#pragma once

#include "std.h"
#include "glm.h"

namespace framework {
  //--------------------------------------------------------------------------
  // 2D Hammersley Point Set
  //--------------------------------------------------------------------------

  namespace detail {
    // Compute the base 2 Van der Corput sequence (aka bit reversal).
    // http://mathworld.wolfram.com/vanderCorputSequence.html
    inline float radical_inverse(uint32_t b) {
      b = _byteswap_ulong(b);
      b = ((b & 0x0F0F0F0Fu) << 4u) | ((b & 0xF0F0F0F0u) >> 4u);
      b = ((b & 0x33333333u) << 2u) | ((b & 0xCCCCCCCCu) >> 2u);
      b = ((b & 0x55555555u) << 1u) | ((b & 0xAAAAAAAAu) >> 1u);
      return float(b) * 2.3283064365386963e-10f; // 1 / 0x100000000
    }
  }

  
  // 2D Hammersley Point Set
  // http://mathworld.wolfram.com/HammersleyPointSet.html
  inline vec2 hammersley_2d(uint32_t i, uint32_t N) {
    return vec2(i * (1.0 / N), detail::radical_inverse(i));
  }
}