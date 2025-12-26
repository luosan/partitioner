#include "tclmanager.h"
#include "cmdlog.h"
#include "register.h"
#include "version.h" // 包含生成的版本头文件
#include <database/database.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/MessageOnlyFormatter.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Log.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <tclDecls.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#if defined(__linux__) || defined(__FreeBSD__)
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#include <sys/user.h>
#endif

#if !defined(_WIN32) || defined(__MINGW32__)
#include <unistd.h>
#else
char *optarg;
int optind = 1, optcur = 1;
int getopt(int argc, char **argv, const char *optstring) {
  if (optind >= argc || argv[optind][0] != '-')
    return -1;

  bool takes_arg = false;
  int opt = argv[optind][optcur];
  for (int i = 0; optstring[i]; i++)
    if (opt == optstring[i] && optstring[i + 1] == ':')
      takes_arg = true;

  if (!takes_arg) {
    if (argv[optind][++optcur] == 0)
      optind++, optcur = 1;
    return opt;
  }

  if (argv[optind][++optcur]) {
    optarg = argv[optind++] + optcur;
    optcur = 1;
    return opt;
  }

  optarg = argv[++optind];
  optind++, optcur = 1;
  return opt;
}
#endif

using namespace Shell;

// 定义版本字符串
const char *lec_version_str =
    LEC_CASED_ID " " LEC_VERSION " (git sha1 " LEC_GIT_VERSION ")";

// 静态成员变量定义
static Tcl_Interp *m_tcl_interp = NULL;
TclManager* TclManager::m_instance = nullptr;
std::mutex TclManager::m_mutex;
bool TclManager::m_initialized = false;
int TclManager::s_argc = 0;
char** TclManager::s_argv = nullptr;

// 私有构造函数 - 不接受参数
TclManager::TclManager() {
  m_tcl_interp = Tcl_CreateInterp();
}

// 单例实例获取方法 - 懒汉式，线程安全
TclManager *TclManager::instance() {
  // 使用双重检查锁定模式确保线程安全
  if (m_instance == nullptr) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_instance == nullptr) {
      m_instance = new TclManager();
    }
  }
  return m_instance;
}

// 初始化方法，只能调用一次
void TclManager::init(int argc, char **argv) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_initialized) {
    s_argc = argc;
    s_argv = argv;
    m_initialized = true;
  
  }
}

int TclManager::tclIterpInit(Tcl_Interp *interp) {
  DataBase::DesignSet::init();
  Command::initRegister();

  if (Tcl_Init(interp) != TCL_OK) {
    CmdLog::logWarning("Tcl_Init() call failed - %s",
                       Tcl_ErrnoMsg(Tcl_GetErrno()));
  }
  for (const auto &cmd : cmd_register) {
    Tcl_CreateCommand(interp, cmd.first.c_str(), executeCmd, NULL, NULL);
  }

  return TCL_OK;
}

Tcl_Interp *TclManager::tclInterp() { return m_tcl_interp; }

int TclManager::executeCmd(ClientData, Tcl_Interp *interp, int argc,
                           const char *argv[]) {
  try {

    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i) {
      args.emplace_back(argv[i]);
    }
    Command::call(args);
    return TCL_OK;

  } catch (Shell::cmd_error_exception) {
    Tcl_SetResult(interp, (char *)"Command produced an error", TCL_STATIC);
    return TCL_ERROR;
  } catch (...) {
    CmdLog::logError("uncaught exception during command invoked from TCL");
    return TCL_ERROR;
  }
}


void TclManager::printHelp(char **argv) {

  CmdLog::logInfo("Usage: %s [options] [<infile> [..]]", argv[0]);
  CmdLog::logInfo("    -l logfile");
  CmdLog::logInfo("        write log messages to the specified file");
  CmdLog::logInfo("    -s scriptfile");
  CmdLog::logInfo("        execute the commands in the script file");
  CmdLog::logInfo("    -d");
  CmdLog::logInfo("        print more detailed timing stats at exit");
  CmdLog::logInfo("When no commands, script files or input files are specified "
                  "on the command");
  CmdLog::logInfo("line, lec automatically enters the interactive command "
                  "mode. Use the 'help'");
  CmdLog::logInfo("command to get information on the individual commands.");
}

void TclManager::run() {
   static plog::ConsoleAppender<plog::MessageOnlyFormatter>
      consoleAppender;                      // Create the 2nd appender.
  plog::init(plog::info, &consoleAppender); // initialize the logger
   
  int argc = s_argc;
  char **argv = s_argv;
  std::string scriptfile = "";
  std::vector<std::string> passes_commands;

  if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-help") ||
                    !strcmp(argv[1], "--help"))) {
    printHelp(argv);
    exit(0);
  }

  if (argc == 2 && (!strcmp(argv[1], "-V") || !strcmp(argv[1], "-version") ||
                    !strcmp(argv[1], "--version"))) {
    CmdLog::logInfo("%s", lec_version_str);
    exit(0);
  }

  bool show_gui = false;
  for (int i = 0; i < argc; ++i) {
    if (!strcmp(argv[i], "-gui")) {
      show_gui = true;
    }
  }

  int opt;
  while ((opt = getopt(
              argc, argv,
              "MXAQTVCSgm:f:Hh:b:o:p:l:L:qv:tds:c:W:w:e:r:D:P:E:x:B:")) != -1) {
    switch (opt) {
    case 'l':
    case 'L': {
      char *logfile = optarg;
      // Check if file exists and truncate it if it does
      std::ifstream check_file(logfile);
      if (check_file.good()) {
        check_file.close();
        std::ofstream truncate_file(logfile, std::ios::trunc);
        truncate_file.close();
      }

      CmdLog::addFileAppender(logfile);
      break;
    }
    case 'd':
      //            timing_details = true;
      break;
    case 's':
      scriptfile = optarg;
      break;
    default:
      fprintf(stderr, "Run '%s -h' for help.", argv[0]);
      exit(1);
    }
  }
  if (!scriptfile.empty()) {
    tclIterpInit(tclInterp());
    if (Tcl_EvalFile(tclInterp(), scriptfile.c_str()) != TCL_OK) {
      CmdLog::logError("TCL interpreter returned an error: %s\n",
                      Tcl_GetStringResult(tclInterp()));
    }
  }
  else {
    lec_banner();
    Tcl_Main(argc, argv, tclIterpInit);
  }
}


void TclManager::shutdown() {

}

void TclManager::lec_banner() {
  CmdLog::logInfo(" /"
                  "------------------------------------------------------------"
                  "----------------\\");
  CmdLog::logInfo(" |                                                          "
                  "                  |");
  CmdLog::logInfo(" |  LEC -- Logic Equivalence Check             "
                  "                  |");
  CmdLog::logInfo(" |                                                          "
                  "                  |");
  CmdLog::logInfo(" |  Copyright (C) 2023 - 2025  <easyformal@gmail.com>       "
                  "                  |");
  CmdLog::logInfo(" |                                                          "
                  "                  |");
  CmdLog::logInfo(" |                                                          "
                  "                  |");
  CmdLog::logInfo(" \\---------------------------------------------------------"
                  "-------------------/");
  CmdLog::logInfo(" %s", lec_version_str);
}


TclManager::~TclManager() {
  shutdown();
  // 释放 TCL 解释器
  if (m_tcl_interp) {
    Tcl_DeleteInterp(m_tcl_interp);
    m_tcl_interp = nullptr;
  }
  Tcl_Finalize();
  
  // 清除静态实例指针（线程安全）
  std::lock_guard<std::mutex> lock(m_mutex);
  m_instance = nullptr;
  m_initialized = false;
}
