#include "stdafx.h"
#include "epoch.h"

#if 0

namespace framework {
  epoch_record::epoch_record(epoch_manager & global) 
  : global(global) 
  , used(true)
  , active(0)
  , epoch(0)
  , dispatch_count(0)
  , peak(0)
  , pending_count(0)
  , pending{ nullptr, nullptr, nullptr, nullptr } {
    atomic_thread_fence(std::memory_order_release);
    // cons onto global list, upmc
    do {
      next = global.records.load(std::memory_order_relaxed);
      atomic_thread_fence(std::memory_order_release);
    } while (!global.records.compare_exchange_weak(next, this, std::memory_order_relaxed));
  }
}

#endif