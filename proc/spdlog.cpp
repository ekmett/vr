#include "stdafx.h"

#include "spdlog.h"

using namespace std;

static std::mutex factory_mutex;

namespace framework {
  namespace logging {
    shared_ptr<logger> default_factory(const char * name) {
      std::lock_guard<std::mutex> guard(factory_mutex);
      // double check lock
      auto result = spdlog::get(name);
      if (!result) result = create<default_sink>(name);
      return result;
    }
   
    shared_ptr<logger>(*factory)(const char * name) = default_factory;

  }
}