#pragma once

#include "framework/std.h"

namespace framework {

  template <typename T>
  struct alloca_array {
    typedef T value_type;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    alloca_array(std::size_t N) : elems(new T[N]), N(N) {}
    ~alloca_array() {
      delete[] elems;
    }

    iterator begin() { return elems; }
    const_iterator begin() const { return elems; }
    iterator end() { return elems + N; }
    const_iterator end() const { return elems + N; }

    reverse_iterator rbegin() { return std::make_reverse_iterator<iterator>(end()); }
    const_reverse_iterator rbegin() const { return std::make_reverse_iterator<const_iterator>(end()); }
    reverse_iterator rend() { return std::make_reverse_iterator<iterator>(begin()); }
    const_reverse_iterator rend() const { return std::make_reverse_iterator<const_iterator>(begin()); }

    size_type size() const noexcept {
      return N;
    }
    bool empty() const noexcept {
      return N == 0;
    }
    size_type max_size() const noexcept {
      return N;
    }
    reference operator[](size_type i) noexcept {
      return elems[i];
    }
    const_reference operator[](size_type i) const noexcept {
      return elems[i];
    }
    reference at(size_type i) noexcept {
      return elems[i];
    }
    const_reference at(size_type i) const noexcept {
      return elems[i];
    }
    reference front() noexcept {
      return elems[0];
    }
    const_reference front() const noexcept {
      return elems[0];
    }
    reference back() noexcept {
      return elems[N - 1];
    }
    const_reference back() const noexcept {
      return elems[N - 1];
    }
    const T* data() const {
      return elems;
    }
    T * c_array() {
      return elems;
    }
    void swap(alloca_array & that) {
      std::swap(elems, that.elems);
      std::swap(N, that.N);
    }

    void assign(const T& value) {
      std::fill_n(begin(), N, value);
    }

    T * elems;
    size_t N;
  };
}