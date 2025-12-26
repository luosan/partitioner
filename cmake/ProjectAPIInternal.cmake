function(add_project_executable name)
  cmake_parse_arguments(_arg "SKIP_INSTALL;SKIP_TRANSLATION;ALLOW_ASCII_CASTS;SKIP_PCH;QTC_RUNNABLE"
  "DESTINATION;COMPONENT;BUILD_DEFAULT"
  "CONDITION;DEPENDS;DEFINES;INCLUDES;SOURCES;EXPLICIT_MOC;SKIP_AUTOMOC;EXTRA_TRANSLATIONS;PROPERTIES" ${ARGN})

  set(_DESTINATION "${MY_PROJECT_APP_PATH}")
  if (_arg_DESTINATION)
    set(_DESTINATION "${_arg_DESTINATION}")
  endif()
  set(_EXECUTABLE_PATH "${_DESTINATION}")

  add_executable("${name}" ${_arg_SOURCES})

  extend_project_target("${name}"
    INCLUDES "${CMAKE_BINARY_DIR}/src" ${_arg_INCLUDES}
    DEPENDS ${_arg_DEPENDS} ${IMPLICIT_DEPENDS}
    DEFINES  ${_arg_DEFINES}
  )

  file(RELATIVE_PATH relative_lib_path "/${_EXECUTABLE_PATH}" "/${MY_PROJECT_LIBRARY_PATH}")

  set(build_rpath "${MY_PROJECT_RPATH_BASE}/${relative_lib_path}")
  set(install_rpath "${MY_PROJECT_RPATH_BASE}/${relative_lib_path}")

  set(build_rpath "${build_rpath};${CMAKE_BUILD_RPATH}")
  set(install_rpath "${install_rpath};${CMAKE_INSTALL_RPATH}")

  project_output_binary_dir(_output_binary_dir)

  set_target_properties("${name}" PROPERTIES
    LINK_DEPENDS_NO_SHARED ON
    BUILD_RPATH "${build_rpath}"
    INSTALL_RPATH "${install_rpath}"    
    RUNTIME_OUTPUT_DIRECTORY "${_output_binary_dir}/${_DESTINATION}"
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN ON    
    ${_arg_PROPERTIES}
  )

  if (NOT _arg_SKIP_INSTALL)
    unset(COMPONENT_OPTION)
  endif()
  if (_arg_COMPONENT)
    set(COMPONENT_OPTION "COMPONENT" "${_arg_COMPONENT}")
  endif()
  install(TARGETS ${name}
    DESTINATION "${_DESTINATION}"
    ${COMPONENT_OPTION}
    OPTIONAL
  )

endfunction()

function(add_project_library name)
  cmake_parse_arguments(_arg "STATIC;OBJECT;SHARED;INTERFACE;SKIP_TRANSLATION;ALLOW_ASCII_CASTS;FEATURE_INFO;SKIP_PCH;EXCLUDE_FROM_INSTALL"
    "DESTINATION;COMPONENT;SOURCES_PREFIX;BUILD_DEFAULT"
    "CONDITION;DEPENDS;PUBLIC_DEPENDS;DEFINES;PUBLIC_DEFINES;INCLUDES;PUBLIC_INCLUDES;SOURCES;EXPLICIT_MOC;SKIP_AUTOMOC;EXTRA_TRANSLATIONS;PROPERTIES" ${ARGN}
  )

  if (${_arg_UNPARSED_ARGUMENTS})
    message(FATAL_ERROR "add_project_library had unparsed arguments")
  endif()

  set(library_type SHARED)
  if (_arg_INTERFACE)
    set(library_type INTERFACE)
  elseif (_arg_STATIC OR (PROJECT_STATIC_BUILD AND NOT _arg_SHARED))
    set(library_type STATIC)
  elseif (_arg_OBJECT)
    set(library_type OBJECT)
  endif()

  add_library(${name} ${library_type})
  add_library(${MY_PROJECT_CASED_ID}::${name} ALIAS ${name})

  # For INTERFACE libraries, we need to handle properties differently
  if(library_type STREQUAL "INTERFACE")
    # Set the linker language for INTERFACE libraries
    set_target_properties(${name} PROPERTIES
      INTERFACE_COMPILE_FEATURES cxx_std_17
    )
  endif()

  if (${name} MATCHES "^[^0-9-]+$")
    if (PROJECT_STATIC_BUILD)
      set(export_symbol_suffix "STATIC_LIBRARY")
    else()
      set(export_symbol_suffix "LIBRARY")
    endif()
    string(TOUPPER "${name}_${export_symbol_suffix}" EXPORT_SYMBOL)
  endif()
  
  if((_arg_STATIC OR _arg_OBJECT) AND UNIX)
    set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()

  extend_project_target(${name}
    SOURCES_PREFIX ${_arg_SOURCES_PREFIX}
    SOURCES ${_arg_SOURCES}
    INCLUDES ${_arg_INCLUDES}
    PUBLIC_INCLUDES ${_arg_PUBLIC_INCLUDES}
    DEFINES ${_arg_DEFINES}
    PUBLIC_DEFINES ${_arg_PUBLIC_DEFINES}
    DEPENDS ${_arg_DEPENDS} ${IMPLICIT_DEPENDS}
    PUBLIC_DEPENDS ${_arg_PUBLIC_DEPENDS}
  )

  # Only set export symbols for non-INTERFACE libraries
  if(NOT library_type STREQUAL "INTERFACE")
    if (PROJECT_STATIC_BUILD)
      extend_project_target(${name}
        DEFINES ${EXPORT_SYMBOL}
        PUBLIC_DEFINES ${EXPORT_SYMBOL})
    else()
      extend_project_target(${name} DEFINES ${EXPORT_SYMBOL})
      if (_arg_OBJECT OR _arg_STATIC)
        extend_project_target(${name} PUBLIC_DEFINES ${EXPORT_SYMBOL})
      endif()
    endif()
  endif()

  set(_DESTINATION "${MY_PROJECT_APP_PATH}")
  if (_arg_DESTINATION)
    set(_DESTINATION "${_arg_DESTINATION}")
  endif()

  # everything is different with SOURCES_PREFIX
  if (NOT _arg_SOURCES_PREFIX)
    get_filename_component(public_build_interface_dir "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
    file(RELATIVE_PATH include_dir_relative_path ${PROJECT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/..")
    
    if(library_type STREQUAL "INTERFACE")
      target_include_directories(${name}
        INTERFACE
          "$<BUILD_INTERFACE:${public_build_interface_dir}>"
          "$<INSTALL_INTERFACE:${MY_PROJECT_HEADER_INSTALL_PATH}/${include_dir_relative_path}>"
      )
    else()
      target_include_directories(${name}
        PRIVATE
          "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        PUBLIC
          "$<BUILD_INTERFACE:${public_build_interface_dir}>"
          "$<INSTALL_INTERFACE:${MY_PROJECT_HEADER_INSTALL_PATH}/${include_dir_relative_path}>"
      )
    endif()
  endif()

  project_output_binary_dir(_output_binary_dir)
  
  string(REGEX MATCH "^[0-9]*" PROJECT_VERSION_MAJOR ${MY_PROJECT_VERSION})

  # Set properties appropriate for the library type
  if(library_type STREQUAL "INTERFACE")
    # INTERFACE libraries only support limited properties
    set_target_properties(${name} PROPERTIES
      SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
      ${_arg_PROPERTIES}
    )
  else()
    set_target_properties(${name} PROPERTIES
      LINK_DEPENDS_NO_SHARED ON
      SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
      VERSION "${MY_PROJECT_VERSION}"
      SOVERSION "${PROJECT_VERSION_MAJOR}"
      MACHO_CURRENT_VERSION "${MY_PROJECT_VERSION}"
      MACHO_COMPATIBILITY_VERSION "${MY_PROJECT_VERSION_COMPAT}"
      CXX_EXTENSIONS OFF
      CXX_VISIBILITY_PRESET default
      VISIBILITY_INLINES_HIDDEN ON
      BUILD_RPATH "${MY_PROJECT_LIB_RPATH};${CMAKE_BUILD_RPATH}"
      INSTALL_RPATH "${MY_PROJECT_LIB_RPATH};${CMAKE_INSTALL_RPATH}"
      RUNTIME_OUTPUT_DIRECTORY "${_output_binary_dir}/${_DESTINATION}"
      LIBRARY_OUTPUT_DIRECTORY "${_output_binary_dir}/${MY_PROJECT_LIBRARY_PATH}"
      ARCHIVE_OUTPUT_DIRECTORY "${_output_binary_dir}/${MY_PROJECT_LIBRARY_ARCHIVE_PATH}"
      ${_arg_PROPERTIES}
    )
  endif()

  unset(NAMELINK_OPTION)
  if (library_type STREQUAL "SHARED")
    set(NAMELINK_OPTION NAMELINK_SKIP)
  endif()

  unset(COMPONENT_OPTION)
  if (_arg_COMPONENT)
    set(COMPONENT_OPTION "COMPONENT" "${_arg_COMPONENT}")
  endif()

  if (NOT _arg_EXCLUDE_FROM_INSTALL AND (NOT PROJECT_STATIC_BUILD OR _arg_SHARED))
  install(TARGETS ${name}
    EXPORT ${MY_PROJECT_CASED_ID}
    RUNTIME
      DESTINATION "${_DESTINATION}"
      ${COMPONENT_OPTION}
      OPTIONAL
    LIBRARY
      DESTINATION "${MY_PROJECT_LIBRARY_PATH}"
      ${NAMELINK_OPTION}
      ${COMPONENT_OPTION}
      OPTIONAL
    OBJECTS
      DESTINATION "${MY_PROJECT_LIBRARY_PATH}"
      COMPONENT Devel EXCLUDE_FROM_ALL
    ARCHIVE
      DESTINATION "${MY_PROJECT_LIBRARY_ARCHIVE_PATH}"
      COMPONENT Devel EXCLUDE_FROM_ALL
      OPTIONAL
  )
  endif()
endfunction()

function(project_output_binary_dir varName)
  if (PROJECT_MERGE_BINARY_DIR)
    string(CONCAT VAR_NAME ${MY_PROJECT_CASED_ID} "_BINARY_DIR")
    set(${varName} ${${VAR_NAME}} PARENT_SCOPE)
  else()
    set(${varName} ${PROJECT_BINARY_DIR} PARENT_SCOPE)
  endif()
endfunction()

function(extend_project_target target_name)
  cmake_parse_arguments(_arg
  ""
  "SOURCES_PREFIX;SOURCES_PREFIX_FROM_TARGET;"
  "CONDITION;DEPENDS;PUBLIC_DEPENDS;DEFINES;PUBLIC_DEFINES;INCLUDES;PUBLIC_INCLUDES;SOURCES;EXPLICIT_MOC;SKIP_AUTOMOC;EXTRA_TRANSLATIONS;PROPERTIES;SOURCES_PROPERTIES"
  ${ARGN}
  )
  if (${_arg_UNPARSED_ARGUMENTS})
    message(FATAL_ERROR "extend_project_target had unparsed arguments")
  endif()

  condition_info(_extra_text _arg_CONDITION)
  if (NOT _arg_CONDITION)
    set(_arg_CONDITION ON)
  endif()
  if (${_arg_CONDITION})
    set(_feature_enabled ON)
  else()
    set(_feature_enabled OFF)
  endif()
  if (_arg_FEATURE_INFO)
    add_feature_info(${_arg_FEATURE_INFO} _feature_enabled "${_extra_text}")
  endif()
  if (NOT _feature_enabled)
    return()
  endif()

  # Check if target is INTERFACE library
  get_target_property(target_type ${target_name} TYPE)
  
  add_project_depends(${target_name}
    PRIVATE ${_arg_DEPENDS}
    PUBLIC ${_arg_PUBLIC_DEPENDS}
  )

  if(target_type STREQUAL "INTERFACE_LIBRARY")
    # For INTERFACE libraries, only INTERFACE properties are allowed
    target_compile_definitions(${target_name}
      INTERFACE ${_arg_DEFINES} ${_arg_PUBLIC_DEFINES}
    )
    if(_arg_INCLUDES)
      target_include_directories(${target_name} INTERFACE ${_arg_INCLUDES})
    endif()
    if(_arg_SYSTEM_INCLUDES)
      target_include_directories(${target_name} SYSTEM INTERFACE ${_arg_SYSTEM_INCLUDES})
    endif()
    
    set_public_includes(${target_name} "${_arg_PUBLIC_INCLUDES}" "")
    set_public_includes(${target_name} "${_arg_PUBLIC_SYSTEM_INCLUDES}" "SYSTEM")
    
    # INTERFACE libraries cannot have sources, but we can add them as a custom target for IDE
    if(_arg_SOURCES)
      add_custom_target(${target_name}-sources SOURCES ${_arg_SOURCES})
    endif()
  else()
    # For regular libraries, use the original behavior
    target_compile_definitions(${target_name}
      PRIVATE ${_arg_DEFINES}
      PUBLIC ${_arg_PUBLIC_DEFINES}
    )
    target_include_directories(${target_name} PRIVATE ${_arg_INCLUDES})
    target_include_directories(${target_name} SYSTEM PRIVATE ${_arg_SYSTEM_INCLUDES})

    set_public_includes(${target_name} "${_arg_PUBLIC_INCLUDES}" "")
    set_public_includes(${target_name} "${_arg_PUBLIC_SYSTEM_INCLUDES}" "SYSTEM")

    target_sources(${target_name} PRIVATE ${_arg_SOURCES})
  endif()

endfunction()

function(set_public_includes target includes system)
  # Check if target is INTERFACE library
  get_target_property(target_type ${target} TYPE)
  
  foreach(inc_dir IN LISTS includes)
    if (NOT IS_ABSOLUTE ${inc_dir})
      set(inc_dir "${CMAKE_CURRENT_SOURCE_DIR}/${inc_dir}")
    endif()
    file(RELATIVE_PATH include_dir_relative_path ${PROJECT_SOURCE_DIR} ${inc_dir})
    
    if(target_type STREQUAL "INTERFACE_LIBRARY")
      target_include_directories(${target} ${system} INTERFACE
        $<BUILD_INTERFACE:${inc_dir}>
        $<INSTALL_INTERFACE:${MY_PROJECT_HEADER_INSTALL_PATH}/${include_dir_relative_path}>
      )
    else()
      target_include_directories(${target} ${system} PUBLIC
        $<BUILD_INTERFACE:${inc_dir}>
        $<INSTALL_INTERFACE:${MY_PROJECT_HEADER_INSTALL_PATH}/${include_dir_relative_path}>
      )
    endif()
  endforeach()
endfunction()

function(add_project_depends target_name)
  cmake_parse_arguments(_arg "" "" "PRIVATE;PUBLIC" ${ARGN})
  if (${_arg_UNPARSED_ARGUMENTS})
    message(FATAL_ERROR "add_project_depends had unparsed arguments")
  endif()

  set(depends "${_arg_PRIVATE}")
  set(public_depends "${_arg_PUBLIC}")

  get_target_property(target_type ${target_name} TYPE)
  if (target_type STREQUAL "INTERFACE_LIBRARY")
    # For INTERFACE libraries, all dependencies become INTERFACE dependencies
    target_link_libraries(${target_name} INTERFACE ${depends} ${public_depends})
  elseif (NOT target_type STREQUAL "OBJECT_LIBRARY")
    target_link_libraries(${target_name} PRIVATE ${depends} PUBLIC ${public_depends})
  else()
    list(APPEND object_lib_depends ${depends})
    list(APPEND object_public_depends ${public_depends})
  endif()

  foreach(obj_lib IN LISTS object_lib_depends)
    target_compile_options(${target_name} PRIVATE $<TARGET_PROPERTY:${obj_lib},INTERFACE_COMPILE_OPTIONS>)
    target_compile_definitions(${target_name} PRIVATE $<TARGET_PROPERTY:${obj_lib},INTERFACE_COMPILE_DEFINITIONS>)
    target_include_directories(${target_name} PRIVATE $<TARGET_PROPERTY:${obj_lib},INTERFACE_INCLUDE_DIRECTORIES>)
  endforeach()
  foreach(obj_lib IN LISTS object_public_depends)
    target_compile_options(${target_name} PUBLIC $<TARGET_PROPERTY:${obj_lib},INTERFACE_COMPILE_OPTIONS>)
    target_compile_definitions(${target_name} PUBLIC $<TARGET_PROPERTY:${obj_lib},INTERFACE_COMPILE_DEFINITIONS>)
    target_include_directories(${target_name} PUBLIC $<TARGET_PROPERTY:${obj_lib},INTERFACE_INCLUDE_DIRECTORIES>)
  endforeach()
endfunction()

function(condition_info varName condition)
  if (NOT ${condition})
    set(${varName} "" PARENT_SCOPE)
  else()
    string(REPLACE ";" " " _contents "${${condition}}")
    set(${varName} "with CONDITION ${_contents}" PARENT_SCOPE)
  endif()
endfunction()