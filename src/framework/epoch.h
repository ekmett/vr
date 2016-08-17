#pragma once

#include <cassert>
#include "framework/std.h";

// Epoch-based reclamation

// Based on
// [Practical Lock-freedom](https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-579.pdf)
// by Keir Fraser 
// (Found in Section 5.2.3)

namespace framework {

  struct collector;

  struct retired_ptr {
    retired_ptr(void * item, void(*finalizer)(void *)) : item(item), finalizer(finalizer) {}
    ~retired_ptr() {
      finalizer(item);
    }
    void * item;
    void(* finalizer) (void *);
    unique_ptr<retired_ptr> next;
  };

  enum struct Epoch : int {
    min = 0,
    max = 2,
    inactive = 3,
    detached = 4
  };

  struct collector_local {
    collector_local(collector & global) noexcept
    : epoch(Epoch::inactive),
      next(nullptr),
      global(global) {}

    void detach() noexcept {
      epoch.store(Epoch::detached, std::memory_order_release);
    }

    // I'd use operator++(int) but msvc seems to have an issue.
    Epoch succ(Epoch e) {
      return Epoch((int(e) + 1) % 3);
    }

    friend collector;
    atomic<Epoch> epoch; // 0,1,2 for current epoch, 3 if inactive.
    atomic<collector_local*> next;
    atomic<retired_ptr*> limbo[3];
    collector & global;
    

    void retire(void * v, void(*f)(void*)) {
      retired_ptr * it = new retired_ptr{ v, f };
      // thread it onto the right limbo list.
    }
   
    void access_lock() noexcept {
      assert(epoch.load(std::memory_order_relaxed) >= Epoch::inactive); // we better not already be running
      epoch.store(global.epoch.load(std::memory_order_relaxed), std::memory_order_relaxed);
      atomic_thread_fence(std::memory_order_acquire);      
    }

    void access_unlock() noexcept {
      assert(epoch.load(std::memory_order_relaxed) < Epoch::inactive);
      atomic_thread_fence(std::memory_order_release);
      epoch.store(Epoch::inactive, std::memory_order_relaxed);
    }

    void collect(Epoch gc) {


    }

    bool sync() {
      atomic<collector_local*>* last = &global.local;
      Epoch now = global.epoch.load(std::memory_order_relaxed);
      collector_local * t = global.local.load(std::memory_order_relaxed);
      while (t) {
        Epoch then = t->epoch.load(std::memory_order_relaxed);
        collector_local * n = t->next.load(std::memory_order_relaxed);
        if (then <= Epoch::max && then != now) {
          collect(succ(global.epoch.load(std::memory_order_relaxed)));
          return false;
        }
        if (then == Epoch::detached && last->compare_exchange_strong(t, n, std::memory_order_relaxed)) {
          retire(t, operator delete);          
        }
        last = &t->next;
        t = n;
      }
      global.epoch.store(succ(now), std::memory_order_relaxed);
      collect(succ(global.epoch.load(std::memory_order_relaxed)));
      return true;
    }
  };

  struct collector {
    atomic<Epoch> epoch; // 0, 1 or 2.
    atomic<collector_local*> local;

    collector_local * attach() {
      collector_local * it = new collector_local();
      collector_local * head;
      do {
        head = local;        
        it->next = head;
      } while (!local.compare_exchange_weak(head, it));
      return it;
    }
  };
};