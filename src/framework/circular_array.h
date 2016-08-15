#pragma once

#include "std.h"
#include "aligned_allocator.h"

namespace framework {

  static const int cacheline_size = 64;

  template <typename T, typename Allocator = aligned_allocator<atomic<T>, 128>>
  struct circular_array {
    // We can fix this later with a bunch of placement news, but it is expensive and I'm lazy.
    static_assert(sizeof(T) == sizeof(atomic<T>));

    size_t N;
    Allocator allocator;

    circular_array(size_t N) : N(N), allocator() {
      assert(N > 0);
      assert(N&~N == N); // N is a power of two
      allocator.allocate(N);
    }
    size_t size() const noexcept {
      return N;
    }
    T get(size_t index) {
      return items[index & (size() - 1)].load(std::memory_order_relaxed);
    }
    void put(size_t index, T x) {
      items[index & (size() - 1)].store(x, std::memory_order_relaxed);
    }
    circular_array * grow(size_t top, size_t bottom) {
      circular_array * new_array = new circular_array(N * 2);
      new_array->previous.reset(this);
      for (std::size_t i = top; i != bottom; ++i)
        new_array->put(i, get(i));
      return new_array;
    }
  private:
    atomic<T*> * items;
    aligned_array<atomic<T*>, 64> items;
    unique_ptr<circular_array> previous;
  };
}