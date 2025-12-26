set(MY_PROJECT_VERSION "0.0.1")
set(MY_PROJECT_ID "easypart")
set(MY_PROJECT_CASED_ID "EasyPart")

# Find Git and get commit hash
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE MY_PROJECT_GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_RESULT
  )
  if(NOT GIT_RESULT EQUAL 0)
    set(MY_PROJECT_GIT_VERSION "unknown")
  endif()
else()
  set(MY_PROJECT_GIT_VERSION "unknown")
endif()

# Pass the Git version to C++ code
add_definitions(-DPROJECT_GIT_VERSION="${MY_PROJECT_GIT_VERSION}")