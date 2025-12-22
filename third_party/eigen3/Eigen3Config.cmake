# Eigen3Config.cmake - Local Eigen3 configuration
# This file allows CMake to find_package(Eigen3) from local third_party

if(NOT TARGET Eigen3::Eigen)
  add_library(Eigen3::Eigen INTERFACE IMPORTED)
  set_target_properties(Eigen3::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}"
  )
endif()

set(Eigen3_FOUND TRUE)
set(EIGEN3_FOUND TRUE)
set(EIGEN3_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(EIGEN3_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}")
set(Eigen3_VERSION "3.4.0")
