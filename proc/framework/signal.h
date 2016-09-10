#pragma once

// I'd push and pop this, but as they are templates this blows up all over use-sites, not here.
#ifdef _WIN32
#pragma warning(disable : 4996)
#endif
#include <boost/signals2.hpp>

namespace framework {
  using namespace boost::signals2;
}