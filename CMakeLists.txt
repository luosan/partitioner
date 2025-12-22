cmake_minimum_required(VERSION 3.16)
project(TritonPartStandalone VERSION 1.0.0)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Options
option(BUILD_TESTS "Build tests" ON)
option(USE_ORTOOLS "Use Google OR-Tools for ILP" ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(Threads REQUIRED)

# Find Boost - prefer local third_party/boost if system boost not found
option(USE_LOCAL_BOOST "Use local boost headers from third_party/boost" ON)

if(USE_LOCAL_BOOST OR NOT EXISTS "/usr/include/boost")
  # Use local boost headers
  if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/boost")
    message(STATUS "Using local Boost headers from third_party/boost")
    set(BOOST_INCLUDEDIR "${CMAKE_SOURCE_DIR}/third_party")
    add_library(Boost::boost INTERFACE IMPORTED)
    set_target_properties(Boost::boost PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party"
    )
    set(Boost_FOUND TRUE)
  else()
    message(FATAL_ERROR "Local Boost not found in third_party/boost. Please copy boost headers there.")
  endif()
else()
  # Try system Boost
  find_package(Boost QUIET)
  if(NOT Boost_FOUND)
    # Fall back to local
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/boost")
      message(STATUS "System Boost not found, using local Boost headers from third_party/boost")
      add_library(Boost::boost INTERFACE IMPORTED)
      set_target_properties(Boost::boost PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party"
      )
      set(Boost_FOUND TRUE)
    else()
      message(FATAL_ERROR "Boost not found. Install system boost or copy headers to third_party/boost")
    endif()
  else()
    message(STATUS "Found system Boost: ${Boost_INCLUDE_DIRS}")
  endif()
endif()

# Find Eigen3 - prefer local third_party/eigen3 if system Eigen3 not found
option(USE_LOCAL_EIGEN3 "Use local Eigen3 headers from third_party/eigen3" ON)

if(USE_LOCAL_EIGEN3 OR USE_LOCAL_BOOST)
  # Use local Eigen3 headers
  if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/eigen3/Eigen")
    message(STATUS "Using local Eigen3 headers from third_party/eigen3")
    add_library(Eigen3::Eigen INTERFACE IMPORTED)
    set_target_properties(Eigen3::Eigen PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/eigen3"
    )
    set(Eigen3_FOUND TRUE)
  else()
    message(FATAL_ERROR "Local Eigen3 not found in third_party/eigen3")
  endif()
else()
  find_package(Eigen3 QUIET)
  if(NOT Eigen3_FOUND)
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/eigen3/Eigen")
      message(STATUS "System Eigen3 not found, using local Eigen3 from third_party/eigen3")
      add_library(Eigen3::Eigen INTERFACE IMPORTED)
      set_target_properties(Eigen3::Eigen PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/eigen3"
      )
      set(Eigen3_FOUND TRUE)
    else()
      message(FATAL_ERROR "Eigen3 not found. Install system eigen3 or copy to third_party/eigen3")
    endif()
  else()
    message(STATUS "Found system Eigen3: ${EIGEN3_INCLUDE_DIR}")
  endif()
endif()

# Try to find ortools
if(USE_ORTOOLS)
  # Try system installation
  find_package(ortools QUIET)
  if(ortools_FOUND)
    message(STATUS "Found system ortools")
  else()
    message(WARNING "ortools not found, ILP features will be disabled")
    message(STATUS "  Looked in: ${ORTOOLS_BUILD_DIR}/lib")
    message(STATUS "  Please build OR-Tools first: cd lib/or-tools && mkdir build && cd build && cmake .. && make")
    set(USE_ORTOOLS OFF)
  endif()
endif()

add_subdirectory(lib)
add_subdirectory(src)

# Add examples/tests (disabled for now)
# if(BUILD_TESTS)
#   add_subdirectory(examples)
# endif()

# Summary
message(STATUS "")
message(STATUS "TritonPart Configuration Summary")
message(STATUS "================================")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "Build tests:       ${BUILD_TESTS}")
message(STATUS "Use ortools:       ${USE_ORTOOLS}")
message(STATUS "")
