#pragma once

#include "framework/fmt.h"

namespace framework {  
  template <typename ... Args, typename E = std::runtime_error> __declspec(noreturn) void die(const char * format, const Args & ... args) /* throw(E) */ {
    string message = fmt::format(format, args...);
    log("error")->critical(message);
    throw E(message);
  }
}