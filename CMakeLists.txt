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

# Find Boost
find_package(Boost REQUIRED)

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
