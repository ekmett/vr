#pragma once

#include <chrono>
#include <ratio>
#include "spdlog.h" // using the header-only version of fmtlib bundled with spdlog
#include "grammar.h"

namespace framework {
  using namespace fmt;

  namespace detail {
    static inline const char * show_std_ratio(intmax_t N, intmax_t D) noexcept {
      if (N == 1) {
        switch (D) {
          // case 1000000000000000000000000LL: return "yocto";
          // case 1000000000000000000000LL: return "zepto";
          case 1000000000000000000: return "atto";
          case 1000000000000000: return "femto";
          case 1000000000000: return "pico";
          case 1000000000: return "nano";
          case 1000000: return "micro";
          case 1000: return "milli";
          case 100: return "centi";
          case 10: return "deci";
          case 1: return "";
        }
      }
      if (D == 1) {
        switch (N) {
          case 10: return "deca";
          case 100: return "hecto";
          case 1000: return "kilo";
          case 1000000: return "mega";
          case 1000000000: return "giga";
          case 1000000000000: return "tera";
          case 1000000000000000: return "peta";
          case 1000000000000000000: return "exa";
          // case 1000000000000000000000: return "zeta";
          // case 1000000000000000000000000: return "yotta";
        }
      }
      return "non-SI ratio";
    }
    static inline const char * show_engineering_prefix(intmax_t N, intmax_t D) noexcept {
      if (N == 1) {
        switch (D) {
          // case 1000000000000000000000000LL: return "y";
          // case 1000000000000000000000LL: return "z";
          case 1000000000000000000: return "a";
          case 1000000000000000: return "f";
          case 1000000000000: return "p";
          case 1000000000: return "n";
          case 1000000: return "u"; // "μ";
          case 1000: return "m";
          case 100: return "c";
          case 10: return "d";
          case 1: return "";
        }
      }
      if (D == 1) {
        switch (N) {
          case 10: return "da";
          case 100: return "h";
          case 1000: return "k";
          case 1000000: return "M";
          case 1000000000: return "G";
          case 1000000000000: return "T";
          case 1000000000000000: return "P";
          case 1000000000000000000: return "E";
            // case 1000000000000000000000: return "Z";
            // case 1000000000000000000000000: return "Y";
        }
      }
      return "non-SI ratio";
    }

  }
}

namespace std {

  template <typename OStream, intmax_t Num, intmax_t Den> OStream & operator<<(OStream &os, const std::ratio<Num, Den> &r) {
    return os << framework::detail::show_std_ratio(r);
  }

  namespace chrono {
    /*
    template<typename OStream, typename T, intmax_t N, intmax_t D> OStream& operator<<(OStream& os, const std::chrono::duration<T, std::ratio<N,D>> &c) {
      return os << c.count() << " " << framework::detail::show_std_ratio(N,D) << framework::plural(c.count(), "second", "seconds");
    }
    */
    template<typename OStream, typename T, intmax_t N, intmax_t D> OStream& operator<<(OStream& os, const std::chrono::duration<T, std::ratio<N, D>> &c) {
      return os << c.count() << " " << framework::detail::show_engineering_prefix(N, D) << "s";
    }
  }
}  