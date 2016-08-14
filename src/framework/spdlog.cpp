#include "framework/stdafx.h"
#include "framework/spdlog.h"

using namespace std;

namespace framework {
  namespace logging {
    shared_ptr<logger> default_factory(const char * name) {
      return create<default_sink>(name);
    }
   
    shared_ptr<logger>(*factory)(const char * name) = default_factory;

  }
}