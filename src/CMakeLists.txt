# SPDX-License-Identifier: BSD-3-Clause
# Standalone TritonPart build

option(LOAD_CPLEX "Load CPLEX" OFF)
if (LOAD_CPLEX)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_MODULE_PATH               "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
  set(CMAKE_CXX_FLAGS                 "${CMAKE_CXX_FLAGS} -DIL_STD -m64 -Wall -fPIC")
  set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX   "-isystem ")
  set(LINKER_OPTIONS                  "-Wl,--export-dynamic")
  find_package(Cplex)
  include_directories(SYSTEM ${CPLEX_INCLUDE_DIRS})
endif (LOAD_CPLEX)

# Find required packages (Boost is handled by parent CMakeLists.txt)
find_package(Threads REQUIRED)

# OR-Tools is handled by parent CMakeLists.txt from source
# USE_ORTOOLS variable is set by parent

# Core TritonPart library (algorithms only, no OpenROAD deps)
add_library(tritonpart_core STATIC
  src/Hypergraph.cpp
  src/Utilities.cpp
  src/Coarsener.cpp
  src/Multilevel.cpp
  src/Refiner.cpp
  src/Partitioner.cpp
  src/Evaluator.cpp
  src/GreedyRefine.cpp
  src/ILPRefine.cpp
  src/KWayFMRefine.cpp
  src/KWayPMRefine.cpp
  src/PriorityQueue.cpp
)

target_include_directories(tritonpart_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(tritonpart_core
  PUBLIC
    Threads::Threads
    Boost::boost
)

# Add ortools if available
if(USE_ORTOOLS)
  target_link_libraries(tritonpart_core PUBLIC ortools::ortools)
  target_compile_definitions(tritonpart_core PUBLIC HAS_ORTOOLS)
endif()

# Add CPLEX if available
if(LOAD_CPLEX)
  target_compile_definitions(tritonpart_core PRIVATE LOAD_CPLEX)
  target_link_libraries(tritonpart_core
    PRIVATE
      ${CPLEX_LIBRARIES}
      ${CMAKE_THREAD_LIBS_INIT}
      dl
  )
endif()

# OpenSTA adapter library
add_library(tritonpart_adapter STATIC
  adapter/OpenStaAdapter.cpp
  adapter/OpenStaAdapter_timing.cpp
  TritonPartCore.cpp
)

target_include_directories(tritonpart_adapter
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/adapter
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/sta/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/sta/include/sta
    ${CMAKE_CURRENT_BINARY_DIR}/../lib/sta/include
    ${CMAKE_CURRENT_BINARY_DIR}/../lib/sta/include/sta
    ${CMAKE_BINARY_DIR}/lib/sta/include
    ${CMAKE_BINARY_DIR}/lib/sta/include/sta
)

target_link_libraries(tritonpart_adapter
  PUBLIC
    tritonpart_core
    ${CMAKE_SOURCE_DIR}/lib/sta/build/sta_swig.a  # Swig TCL bindings - MUST come first
    OpenSTA
    ${TCL_LIBRARY}
)

# Main TritonPart executable
# Supports: partition (triton_part_design), evaluate (evaluate_part_design_solution)
add_executable(tritonpart
  main.cpp
)

target_link_libraries(tritonpart
  PRIVATE
    tritonpart_adapter
    tritonpart_core
)

# Install targets
install(TARGETS tritonpart tritonpart_core
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
)

install(DIRECTORY include/par
  DESTINATION include
)
