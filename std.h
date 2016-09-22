#pragma once

#include <atomic> // atomic
#include <memory> // shared_ptr
#include <string> // string
#include <map> // map
#include <set> // set
#include <functional> // mem_fn
#include <mutex>
#include <cassert>
#include <vector>
#include <tuple>
#include <array>

namespace framework {
  using std::atomic;
  using std::shared_ptr;
  using std::unique_ptr;
  using std::string;
  using std::wstring;
  using std::vector;
  using std::map;
  using std::set;
  using std::tuple;
  using std::mem_fn;
  using std::array;
  using std::mutex;
  using std::lock_guard;  
  using std::chrono::duration;

  namespace detail {
    template <typename F, std::size_t... indices>
    constexpr auto make_array_helper(F f, std::index_sequence<indices...>) -> std::array<typename std::result_of<F(std::size_t)>::type, sizeof...(indices)> {
      return{ { f(indices)... } };
    }
  }

  template <int N, typename F>
  constexpr auto make_array(F f) -> std::array<typename std::result_of<F(std::size_t)>::type, N> {
    return detail::make_array_helper(f, std::make_index_sequence<N>{});
  }
}