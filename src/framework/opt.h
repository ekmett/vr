#pragma once

#include "framework/config.h"

#ifdef FRAMEWORK_SUPPORTS_CDS

#include <cds/opt/options.h>

namespace framework {
  namespace opt {
    using namespace cds::opt;
  }
}

#endif