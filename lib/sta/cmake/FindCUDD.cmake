################################################################
#
# Locate CUDD bdd package.
# Prefer local third_party/cudd if available
#
################################################################

# Check for local CUDD first
set(LOCAL_CUDD_DIR "${CMAKE_SOURCE_DIR}/third_party/cudd")
if(EXISTS "${LOCAL_CUDD_DIR}/lib/libcudd.a" AND EXISTS "${LOCAL_CUDD_DIR}/include/cudd.h")
  message(STATUS "Using local CUDD from third_party/cudd")
  set(CUDD_LIB "${LOCAL_CUDD_DIR}/lib/libcudd.a")
  set(CUDD_HEADER "${LOCAL_CUDD_DIR}/include/cudd.h")
  set(CUDD_INCLUDE "${LOCAL_CUDD_DIR}/include")
  message(STATUS "CUDD library: ${CUDD_LIB}")
  message(STATUS "CUDD header: ${CUDD_HEADER}")
else()
  # Fall back to system CUDD
  find_library(CUDD_LIB
    NAME cudd
    PATHS ${CUDD_DIR}
    PATH_SUFFIXES lib lib/cudd cudd/.libs
    )
  if (CUDD_LIB)
    message(STATUS "CUDD library: ${CUDD_LIB}")
    get_filename_component(CUDD_LIB_DIR "${CUDD_LIB}" PATH)
    get_filename_component(CUDD_LIB_PARENT1 "${CUDD_LIB_DIR}" PATH)
    find_file(CUDD_HEADER cudd.h
      PATHS ${CUDD_LIB_PARENT1} ${CUDD_LIB_PARENT1}/include ${CUDD_LIB_PARENT1}/include/cudd)
    if (CUDD_HEADER)
      get_filename_component(CUDD_INCLUDE "${CUDD_HEADER}" PATH)
      message(STATUS "CUDD header: ${CUDD_HEADER}")
    else()
      message(STATUS "CUDD header: not found")
    endif()
  else()
    message(STATUS "CUDD library: not found")
  endif()
endif()
