add_project_library(Shell
  DEPENDS DataBase Plog 
  PUBLIC_DEPENDS Tcl
  SOURCES
  tclmanager.cpp tclmanager.h
  register.cpp register.h
  argument.h argument.cpp
  cmdlog.h cmdlog.cpp
  utils/util.h utils/util.cpp
)
extend_project_target(Shell
  CONDITION WIN32
  DEFINES PLOG_IMPORT
)

extend_project_target(Shell
  CONDITION UNIX
  DEFINES PLOG_GLOBAL
)
