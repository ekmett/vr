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
}