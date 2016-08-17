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
  static const int epoch_mask = 3;

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
    epoch_record(epoch_manager & global);
    const epoch_manager & global;
    atomic<uint32_t> epoch; 
    atomic<bool> used; // if false, we're free and waiting for pickup during scan
    atomic<uint32_t> active;
    cache_isolated<epoch_ref[2]> bucket;
    uint32_t pending_count, peak, dispatch_count; // usage statistics
    epoch_entry * pending[epoch_length];
    epoch_record * next;
    void unregister();
    bool poll() {
      return false;
    }
    void synchronize() {

    }

    void reclaim() {
      for (int i = 0; i < epoch_length; ++i)
        dispatch(epoch);
    }

    void barrier();

    void begin(epoch_section * s = nullptr, int count = 1);
    void end(epoch_section * s = nullptr, int count = 1);

    void retire(epoch_entry & entry);
  private:
    void add_ref(epoch_section & s) {
      // TODO
    }
    void del_ref(epoch_section & s) {
      // TODO
    }
    void dispatch(int epoch) {
      epoch &= epoch_mask;
      int i = 0;
      epoch_entry * head = pending[epoch];
      pending[epoch] = nullptr;
      epoch_entry * n;
      for (epoch_entry * cursor = head; cursor != nullptr; cursor = n) {
        n = cursor->next_epoch_entry;
        delete cursor;
        i++;
      }
      if (pending_count > peak)
        peak = pending_count;
      dispatch_count += i;
      pending_count -= i;
    }
  };

  struct epoch_manager {
    atomic<uint32_t> epoch;
    atomic<int> free_count;
    atomic<epoch_record *> records;
    epoch_manager(): epoch(1), free_count(0), records(nullptr) {
      atomic_thread_fence(std::memory_order_release);
    } // init
    ~epoch_manager() {
      // oh boy
    }
    epoch_record * recycle() const; // try to reuse a freed collector from the list
  };

  inline void epoch_record::begin(epoch_section * section, int count) {    
    assert(count > 0);
    auto a = active.load(std::memory_order_relaxed);
    if (a == 0) {
#ifdef FRAMEWORK_MEMORYMODEL_TSO
      active.exchange(count, std::memory_order_relaxed);
      atomic_thread_fence(std::memory_order_acquire);
#else
      active.store(1, std::memory_order_relaxed);
      atomic_thread_fence(std::memory_order_seq_cst);
#endif
      auto g_epoch = global.epoch.load(std::memory_order_acquire);
      epoch.store(g_epoch, std::memory_order_release);
    } else {
      active.store(a + 1, std::memory_order_relaxed);
    }
    if (section) add_ref(*section);
  }

  inline void epoch_record::barrier() {
    synchronize();
    reclaim();    
  }

  inline void epoch_record::end(epoch_section * section, int count) {
    atomic_thread_fence(std::memory_order_release);
    active.fetch_sub(count, std::memory_order_relaxed);
    if (section) del_ref(*section);
  }

  inline void epoch_record::retire(epoch_entry & entry) {
    int offset = global.epoch.load(std::memory_order_relaxed) & epoch_mask;
    ++pending_count;
    // cons onto the pending list, spnc
    epoch_entry * & p = pending[offset];
    entry.next_epoch_entry = pending[offset];
    pending[offset] = &entry;
  }
  void epoch_record::unregister() {
    // TODO: merge the local free lists into those of whomever is collecting us, then when we're down to one local collector, it can collect itself.
    atomic_thread_fence(std::memory_order_release);
    used.store(false, std::memory_order_relaxed);
    global.free_count.fetch_add(1, std::memory_order_relaxed);
  }

  epoch_record * epoch_manager::recycle() const {
    if (free_count.load(std::memory_order_relaxed) == 0)
      return nullptr;
    for (epoch_record * cursor = records.load(std::memory_order_relaxed); cursor != nullptr; cursor = cursor->next) {
      if (cursor->used.load(std::memory_order_relaxed) == false) {
        atomic_thread_fence(std::memory_order_acquire);
        if (cursor->used.exchange(true, std::memory_order_relaxed)) {
          free_count.fetch_sub(1, std::memory_order_relaxed);
          return cursor;
        }
      }
    }
    return nullptr;
  }


};