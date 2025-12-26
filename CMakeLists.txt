cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ProjectBranding)

project(${MY_PROJECT_CASED_ID} VERSION ${MY_PROJECT_VERSION})

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(ProjectAPI)

add_subdirectory(src)

# Summary
message(STATUS "")
message(STATUS "EasyPart Configuration Summary")
message(STATUS "================================")
message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "")


# # Find required packages
# find_package(Threads REQUIRED)

# # Find Boost (system)
# find_package(Boost REQUIRED)
# message(STATUS "Found Boost: ${Boost_INCLUDE_DIRS}")

# # Find Eigen3 (system)
# find_package(Eigen3 REQUIRED)
# message(STATUS "Found Eigen3: ${EIGEN3_INCLUDE_DIR}")

# # Try to find ortools
# if(USE_ORTOOLS)
#   # Try system installation
#   find_package(ortools QUIET)
#   if(ortools_FOUND)
#     message(STATUS "Found system ortools")
#   else()
#     message(WARNING "ortools not found, ILP features will be disabled")
#     message(STATUS "  Looked in: ${ORTOOLS_BUILD_DIR}/lib")
#     message(STATUS "  Please build OR-Tools first: cd lib/or-tools && mkdir build && cd build && cmake .. && make")
#     set(USE_ORTOOLS OFF)
#   endif()
# endif()

# add_subdirectory(lib)
# add_subdirectory(src)

# # Add examples/tests (disabled for now)
# # if(BUILD_TESTS)
# #   add_subdirectory(examples)
# # endif()

# # Summary
# message(STATUS "")
# message(STATUS "TritonPart Configuration Summary")
# message(STATUS "================================")
# message(STATUS "Build type:        ${CMAKE_BUILD_TYPE}")
# message(STATUS "C++ Standard:      ${CMAKE_CXX_STANDARD}")
# message(STATUS "Build tests:       ${BUILD_TESTS}")
# message(STATUS "Use ortools:       ${USE_ORTOOLS}")
# message(STATUS "")
