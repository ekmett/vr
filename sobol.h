#pragma once

#include <intrin.h>
#include <windows.h>
#include "std.h"

namespace framework {

  //--------------------------------------------------------------------------
  // Sobol Low Discrepancy Sequence
  //--------------------------------------------------------------------------

  namespace detail {
    static inline uint32_t ctz(uint32_t value) {
#ifdef _WIN32
      DWORD trailing_zero = 0;
      if (_BitScanForward(&trailing_zero, value)) {
        return trailing_zero;
      } else {
        // This is undefined, I better choose 32 than 0
        return 32;
      }
#else
      return __builtin_ctz(value);
#endif
    }
    extern const uint32_t sobol_a[1110];
    extern const uint32_t sobol_minit[13][1110];
  }

  template <size_t N = 1>
  struct sobol {
    uint32_t n = 0;
    uint32_t m[N][32];
    uint32_t x[N]{};
    uint32_t b[32]{};

    sobol() {
      static_assert(N <= 1111, "Only 1111 dimensions worth of Sobol sequencing are available");
      for (auto & r : m) for (auto & p : r) p = 1;
      for (uint32_t i = 1;i < N; ++i) {
        uint32_t a = detail::sobol_a[i - 1];
        uint32_t d = floor(log2<float>(a)); // use clz based integer log 2?
        for (uint32_t j = 0;j < d;++j)
          m[i][j] = detail::sobol_minit[j][i - 1]; // TODO: transpose sobol_minit
        for (uint32_t j = d; j < 32; ++j) {
          uint32_t ac = a;
          m[i][j] = m[i][j - d];
          for (uint32_t k = 0;k < d; ++k) {
            m[i][j] ^= ((ac & 1u) * m[i][j - d + k]) << (d - k);
            ac >>= 1;
          }
        }
      }
    }

    inline void next(float data[N]) {
      uint32_t c = detail::ctz(++n);
      for (int i = 0; i < N; ++i) {
        if (b[i] >= c) {
          x[i] ^= m[i][c] << (b[i] - c);
          data[i] = x[i] * exp2(-float(b[i] + 1));
        } else {
          x[i] = (x[i] << (c - b[i])) ^ m[i][c];
          b[i] = c;
          data[i] = x[i] * exp2(-float(c + 1));
        }
      }
    }

    inline void skip(uint32_t delta) {
      uint32_t skips = delta & ~(delta - 1);;
      for (int i = 0;i < skips;++i) next();
    }
  };
}

/*

Sobol sequence generating code is based on code from
https://github.com/stevengj/Sobol.jl, largely just transcoded
into C++ by Edward Kmett, offered under the MIT expat license.

Copyright (c) 2016: Edward A. Kmett.
Copyright (c) 2014: Steven G. Johnson.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/