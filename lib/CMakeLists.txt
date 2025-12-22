# Build OpenSTA as a static library
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sta)
  message(STATUS "Building bundled OpenSTA")
  add_subdirectory(sta EXCLUDE_FROM_ALL)
  
  # Create an interface library for easier usage
  add_library(sta_static INTERFACE)
  target_link_libraries(sta_static INTERFACE OpenSTA)
  target_include_directories(sta_static INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/sta/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sta/include/sta
  )
  
  # Export for parent
  set(OPENSTA_LIBRARY sta_static PARENT_SCOPE)
  set(OPENSTA_FOUND TRUE PARENT_SCOPE)
else()
  message(FATAL_ERROR "OpenSTA source not found in lib/sta. Please copy or link OpenSTA source there.")
endif()

# Note: OR-Tools should be pre-built in lib/or-tools/build
# The main CMakeLists.txt will find and use it
