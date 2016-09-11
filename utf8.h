#pragma once

#include "std.h"

// TODO: portability
#ifdef _WIN32
#include <Windows.h>
#endif

namespace framework {
  static inline std::wstring from_utf8(const char * s) {
    std::wstring ret;
    int dlen = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (dlen > 0) {
      ret.resize(dlen+1);
      MultiByteToWideChar(CP_UTF8, 0, s, -1, const_cast<wchar_t*>(ret.c_str()), dlen);
      ret.resize(dlen);
    }
    return ret;
  }
}
