#pragma once

#include "framework/config.h"

#ifdef FRAMEWORK_SUPPORTS_CDS

#include <cds/init.h>
#include <cds/threading/model.h>
#include <cds/gc/hp.h>
#include <cds/os/topology.h>
#include "framework/noncopyable.h"
#include "framework/spdlog.h"

namespace framework {
  template <typename Collector = cds::gc::hp::GarbageCollector> struct cds_main_thread_attachment : noncopyable {
    cds_main_thread_attachment() {
      cds::Initialize();      
      Collector::Construct();
      cds::threading::Manager::attachThread();
      cds::OS::topology sys_topology;
      log("cds")->info("cds initialized: main thread on processor {} of {}", sys_topology.current_processor(), sys_topology.processor_count());
    }
    ~cds_main_thread_attachment() {
      cds::threading::Manager::detachThread();
      Collector::Destruct();
      cds::Terminate();
    }
    typedef Collector collector_type;
  };

  // before a thread can access any cds data structures, you'll need this.
  struct cds_thread_attachment : noncopyable {
    cds_thread_attachment() { cds::threading::Manager::attachThread(); }
    ~cds_thread_attachment() { cds::threading::Manager::detachThread(); }
  };
}

#endif