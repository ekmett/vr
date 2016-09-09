#include "stdafx.h"
#include "aligned_allocator.h"

namespace framework {
  namespace detail {
    void* allocate_aligned_memory(size_t align, size_t size) {
      assert(align >= sizeof(void*));
      assert(size && ~size == size); // check that the size is a power of two, or 0.
      if (size == 0) return nullptr;
#ifdef _WIN32      
      return _aligned_malloc(size, align);
#else
      void* ptr = nullptr;
      int rc = posix_memalign(&ptr, align, size);
      if (rc != 0) return nullptr;
      return ptr;
#endif      
    }

    void deallocate_aligned_memory(void *ptr) noexcept {
#ifdef _WIN32
      _aligned_free(ptr);
#else
      free(ptr);
#endif
    }
  }
}