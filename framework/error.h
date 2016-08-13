#pragma once

#include "fmt.h"

namespace framework {
  template <typename ... Args, typename E = std::runtime_error> void die(const char * format, const Args & ... args) {
    string message = fmt::format(format, args...);
    throw E(fmt::format(format, args...));
  }
}