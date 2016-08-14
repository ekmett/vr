#pragma once

/*

#include <cds/init.h>
#include <cds/threading/model.h>
#include <cds/gc/hp.h>
#include <cds/os/topology.h>
#include "noncopyable.h"
#include "spdlog.h"

namespace framework {
  template <typename Collector = cds::gc::hp::GarbageCollector> struct cds_main_thread_attachment : noncopyable {
    cds_main_thread_attachment() {
      cds::Initialize();      
      Collector::Construct();
      cds::threading::Manager::attachThread();      
      // log("cds")->info("cds initialized: main thread on processor {} of {}", topology.current_processor(), topology.processor_count());
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

  

  int main(int argc, char **argv) {
    cds_main_thread_attachment<> main_thread_attachment;
    ... spin up another thread to run worker ...
  }

  int worker() {
    cds_thread_attachment worker_thread_attachment;
    ... do stuff ...
  }

  
}

*/