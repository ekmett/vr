#include "stdafx.h"
#include "error.h"
#include "filesystem.h"

namespace framework {
  namespace filesystem {
    path executable_path() {
      bool bSuccess = false;
      char buffer[1024];

      #if defined(_WIN32)

        DWORD buffer_size = sizeof(buffer);
        if (!::GetModuleFileNameA(nullptr, buffer, buffer_size)) {
          die("Unable to obtain module filename: {}", GetLastError());
        }
        return path(buffer);

      #elif defined(OSX)

        uint32_t buffer_size = sizeof(buffer);
        if (_NSGetExecutablePath(buffer, &buffer_size) != 0)
          die("Unable to obtain module filename");
        buffer[buffer_size - 1] = '\0';
        return path(buffer);

      #elif defined(LINUX)

        size_t buffer_size = sizeof(buffer);
        ssize_t len = readlink("/proc/self/exe", buffer, buffer_size - 1);
        if (len == -1)
          die("Unable to obtain module filename");      
        buffer[len] = 0;
        return path(buffer);

      #else

        #error unknown platform

      #endif
    }
  }
}
