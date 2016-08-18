#pragma once

#if 0 // unused

#include "std.h"
#include "aligned_allocator.h"
#include "circular_array.h"
#include "noncopyable.h"

namespace framework {

  static const size_t cache_line_size = 128;

  enum class stealing : int {
    empty = 0,
    stolen = 1,
    aborted = 2
  };

  // a growable, circular, work-stealing deque
  template <typename T, typename Allocator = aligned_allocator<T, cache_line_size>> struct chase_lev_deque : noncopyable {
    chase_lev_deque(size_t initial_size = 32);
    ~chase_lev_deque() noexcept;

    void push(T x); // allocates memory
    inline T pop() noexcept;

    bool pop(T & result) noexcept;
    stealing steal(T & result) noexcept;

  private:
    typedef circular_array<T, Allocator> circular_array_type;

    atomic<circular_array_type *> array;
    atomic<size_t> top, bottom;
  };

  template <typename T, typename Allocator>
  inline chase_lev_deque::chase_lev_deque(size_t initial_size) : array(new circular_array_type(initial_size)), top(0), bottom(0) {}

  template <typename T, typename Allocator>
  inline chase_lev_deque::~chase_lev_deque() noexcept {
    circular_array_type * p = array.load(std::memory_order_relaxed);
    if (p) delete p;
  }

  template <typename T, typename Allocator>
  inline void chase_lev_deque::push(T x) {
    size_t b = bottom.load(std::memory_order_relaxed);
    size_t t = top.load(std::memory_order_acquire);
    circular_array_type * a = array.load(std::memory_order_relaxed);
    if (b - t > a->size() - 1) {
      a = a->grow(t, b);
      array.store(a, std::memory_order_relaxed);
    }
    a->put(b, x);
    std::atomic_thread_fence(std::memory_order_release);
    bottom.store(b + 1, std::memory_order_relaxed);
  }

  template <typename T, typename Allocator>
  inline bool chase_lev_deque::pop(T & result) noexcept {
    size_t b = bottom.load(std::memory_order_relaxed) - 1;
    circular_array_type * a = array.load(std::memory_order_relaxed);
    bottom.store(b, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    size_t t = top.load(std::memory_order_relaxed);
    if (t <= b) {
      T x = a->get(b);
      if (t == b) {
        if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
          return false;
        }
        bottom.store(b + 1, std::memory_order_relaxed);
      }
      result = x;
      return true;
    } else {
      bottom.store(b + 1, std::memory_order_relaxed);
      return false;
    }
  }

  template <typename T, typename Allocator>
  inline T chase_lev_deque::pop() noexcept {
    T result;
    return pop(result) ? result : T();
  }

  template <typename T, typename Allocator>
  inline stealing chase_lev_deque::steal(T & result) noexcept {
    std::size_t t = top.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    std::size_t b = bottom.load(std::memory_order_acquire);
    if (t >= b) return stealing::empty;
    if (t < b) {
      circular_array_type * a = array.load(std::memory_order_relaxed);
      T x = a->get(t);
      if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
        return stealing::aborted;
      }
      result = x;
      return stealing::stolen;
    }
  }

}

#endif