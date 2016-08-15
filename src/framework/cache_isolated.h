#pragma once

namespace framework {

  // try to ensure that T gets its own cache line.
  template <typename T, size_t N = 64> 
  struct cache_isolated {
    static const size_t padding_bytes = N;

    cache_isolated(const cache_isolated<T,N> & that) : data(that.data) {}
    cache_isolated(T && data) : data(std::forward(data)) {}
    template <typename ... T> cache_isolated(T&&...args) : data(std::forward(args)...) {}

    cache_isolated & operator = (cache_isolated that) {
      this.data = that.data;
    }
    cache_isolated & operator = (cache_isolated & that) {
      data = that.data;
      return this;
    }
    cache_isolated & operator = (cache_isolated && that) {
      data.move(that.data);
      return this;
    }

    template <typename T> auto operator = (T && t) { data = t }

  private:
    int8_t padding0[padding_bytes];
  public:
    T data;
  private:
    int8_t padding1[padding_bytes - sizeof(T)];
  };


}