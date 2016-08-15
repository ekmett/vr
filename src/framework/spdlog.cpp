#include "framework/stdafx.h"
#include "framework/spdlog.h"

using namespace std;

static std::mutex factory_mutex;

namespace framework {
  namespace logging {
    shared_ptr<logger> default_factory(const char * name) {
      std::lock_guard<std::mutex> guard(factory_mutex);
      return create<default_sink>(name);
    }
   
    shared_ptr<logger>(*factory)(const char * name) = default_factory;

  }
}