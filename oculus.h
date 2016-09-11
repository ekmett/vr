#pragma once

#include "config.h"

#ifdef FRAMEWORK_SUPPORTS_OCULUS

#include <OVR_CAPI.h>

#ifdef _WIN32
#pragma comment(lib, "LibOVR")
#endif

namespace framework {
  namespace oculus {

  }
}

#endif