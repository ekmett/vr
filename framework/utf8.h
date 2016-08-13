#pragma once

#include "std.h"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace framework {
  static inline std::wstring from_utf8(const char * s) {
    std::wstring ret;
    int slen = (int)strlen(s);
    // use a replacement on other platforms
    int dlen = MultiByteToWideChar(CP_UTF8, 0, s, slen, NULL, 0);
    if (dlen > 0) {
      // evil in-place construction
      ret.resize(dlen);
      MultiByteToWideChar(CP_UTF8, 0, s, slen, const_cast<wchar_t*>(ret.c_str()), dlen);
    }
    return ret;
  }
}