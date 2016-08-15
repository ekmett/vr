cmake_minimum_required (VERSION 3.1.0)
cmake_policy(SET CMP0048 NEW)
project(framework VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

list(APPEND CMAKE_PREFIX_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/third-party/openvr-1.0.2")

include(CheckSymbolExists)
include(FindSDL2)
include(FindOpenVR)

add_definitions(-D_GNU_SOURCE)
add_definitions(-D_LARGEFILE_SOURCE)

# make it easier to add options
macro(add_option var default type docstring)
    if(NOT DEFINED ${var})
        set(${var} ${default})
    endif()
    set(${var} ${${var}} CACHE ${type} ${docstring} FORCE)
endmacro()

# C++11
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX14)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
elseif(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# boost
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
# find_package(Boost 1.45.0 REQUIRED COMPONENTS thread)
# include_directories(${Boost_INCLUDE_DIRS})

# link_libraries(${Boost_LIBRARIES})

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/third-party/spdlog/include)
include_directories(${CMAKE_SOURCE_DIR}/third-party/glm-0.9.8.0)

# sdl

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
set(LIBS ${LIBS} ${SDL2_LIBRARY})
link_libraries(${LIBS})

find_package(OpenVR)

# include_directories(${CMAKE_SOURCE_DIR}/third-party/openvr-1.0.2/include)

# doxygen

# add an option for building the API documentation
add_option(VR_BUILD_DOCUMENTATION TRUE BOOL "Set to TRUE to generate the API documentation. Set to FALSE to skip")

if (VR_BUILD_DOCUMENTATION)
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
    configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc ALL ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
  endif(DOXYGEN_FOUND)
endif(VR_BUILD_DOCUMENTATION)

check_symbol_exists(madvise sys/mman.h HAVE_MADVISE)
if(NOT HAVE_MADVISE)
  check_symbol_exists(posix_madvise sys/mman.h HAVE_POSIX_MADVISE)
endif()

add_subdirectory(src)
