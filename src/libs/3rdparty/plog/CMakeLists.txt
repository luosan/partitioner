set(PLOG_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

add_project_library(Plog
  INTERFACE
  INCLUDES
  ${PLOG_DIR}/include
  PUBLIC_INCLUDES
  ${PLOG_DIR}/include
  SOURCES
  ${PLOG_DIR}/include/plog/Appenders/AndroidAppender.h
  ${PLOG_DIR}/include/plog/Appenders/ConsoleAppender.h
  ${PLOG_DIR}/include/plog/Appenders/EventLogAppender.h
  ${PLOG_DIR}/include/plog/Appenders/ArduinoAppender.h
  ${PLOG_DIR}/include/plog/Appenders/DebugOutputAppender.h
  ${PLOG_DIR}/include/plog/Appenders/IAppender.h
  ${PLOG_DIR}/include/plog/Appenders/ColorConsoleAppender.h
  ${PLOG_DIR}/include/plog/Appenders/DynamicAppender.h
  ${PLOG_DIR}/include/plog/Appenders/RollingFileAppender.h
  ${PLOG_DIR}/include/plog/Converters/NativeEOLConverter.h
  ${PLOG_DIR}/include/plog/Converters/UTF8Converter.h
  ${PLOG_DIR}/include/plog/Helpers/AscDump.h
  ${PLOG_DIR}/include/plog/Helpers/HexDump.h
  ${PLOG_DIR}/include/plog/Helpers/PrintVar.h
  ${PLOG_DIR}/include/plog/Initializers/ConsoleInitializer.h
  ${PLOG_DIR}/include/plog/Initializers/RollingFileInitializer.h
#  Initializers
#  Appenders
#  Formatters
  ${PLOG_DIR}/include/plog/Init.h
  ${PLOG_DIR}/include/plog/Logger.h
  ${PLOG_DIR}/include/plog/Record.h
  ${PLOG_DIR}/include/plog/Util.h
  ${PLOG_DIR}/include/plog/Log.h
  ${PLOG_DIR}/include/plog/Severity.h
  ${PLOG_DIR}/include/plog/WinApi.h
)
