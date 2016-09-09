#pragma once

#include <boost/filesystem.hpp>

//#ifdef _WIN32
//#pragma comment(lib, "boost_filesystem-vc140-mt-gd-1_61")
//#endif

namespace framework {
  namespace filesystem {
    using namespace boost::filesystem;

    extern path executable_path();
  };
};
