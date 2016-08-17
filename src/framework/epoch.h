#pragma once

#include "framework/config.h"
#include <cassert>
#include "framework/std.h"
#include "framework/cache_isolated.h"

// Epoch-based reclamation

// Based on
// [Practical Lock-freedom](https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-579.pdf)
// by Keir Fraser 
// (Found in Section 5.2.3)

// Current implementation lifted almost entirely from Samy Al Bahra's excellent
// ConcurrencyKit. Bugs mine.

// work-in-progress

namespace framework {

  static const int epoch_length = 4;

  struct epoch_section {
    uint32_t bucket;
  };

  // things that inherit from this can be collected
  struct epoch_entry {
    virtual ~epoch_entry() {}
    // TODO: private w/ friends
    epoch_entry * next_epoch_entry;
  };

  struct epoch_manager;

  struct epoch_ref {
    uint32_t epoch;
    uint32_t count;
  };

  struct epoch_record {
    epoch_manager & global;
    atomic<uint32_t> epoch; 
    uint32_t state;
    atomic<uint32_t> active;
    cache_isolated<epoch_ref[2]> bucket;
    uint32_t pending_count, peak, dispatch;
    epoch_entry * pending[epoch_length];   
    void remove() {
    }
    bool poll() {
      return false;
    }
    void synchronize() {}
    void barrier() {}
    void reclaim() {}

    void begin(epoch_section * s = nullptr);
    void end(epoch_section * s = nullptr);

    void retire(epoch_entry & entry);
  private:
    void add_ref(epoch_section & s) {}
    void del_ref(epoch_section & s) {}
  };

  struct epoch_manager {
    atomic<uint32_t> epoch;
    epoch_manager() {} // init
    epoch_record * recycle() { return nullptr; }
    epoch_record * add() {}
  };

  inline void epoch_record::begin(epoch_section * section) {
    epoch_manager & e = global;
    auto a = active.load(std::memory_order_relaxed);
    if (a == 0) {
#ifdef FRAMEWORK_MEMORYMODEL_TSO
      active.exchange(1);
      atomic_thread_fence(std::memory_order_acquire);
#else
      active.store(1);
      atomic_thread_fence(std::memory_order_seq_cst);
#endif
      auto g_epoch = e.epoch.load(std::memory_order_acquire);
      epoch.store(g_epoch, std::memory_order_release);
    } else {
      active.store(a + 1, std::memory_order_relaxed);
    }
    if (section) add_ref(*section);
  }

  inline void epoch_record::end(epoch_section * section) {
    atomic_thread_fence(std::memory_order_release);
    active.fetch_add(1, std::memory_order_relaxed);
    if (section) del_ref(*section);
  }

  inline void epoch_record::retire(epoch_entry & entry) {
    epoch_manager & g = global;
    auto e = g.epoch.load(std::memory_order_relaxed);
    int offset = e & 3;
    ++pending_count;
    epoch_entry * & p = pending[offset];
    entry.next_epoch_entry = pending[offfset];
    pending[offset] = &entry;
  }
};