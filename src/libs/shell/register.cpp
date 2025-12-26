#include "register.h"
#include <errno.h>
#include <plog/Log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argument.h"
#include "tclmanager.h"
#include "utils/util.h"
#if defined(_WIN32)
#include <intrin.h>
#else
#include <sys/resource.h>
#include <sys/time.h>
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <signal.h>
#endif
#endif

#include <database/design.h>
#include <database/designset.h>

struct PerformanceTimer {
#if 1
  int64_t total_ns;

  PerformanceTimer() { total_ns = 0; }

  static int64_t query() {
#ifdef _WIN32
    return 0;
#elif defined(RUSAGE_SELF)
    struct rusage rusage;
    int64_t t = 0;
    for (int who : {RUSAGE_SELF, RUSAGE_CHILDREN}) {
      if (getrusage(who, &rusage) == -1) {
        PLOGE.printf("getrusage failed!");
        return 0;
      }
      t += 1000000000ULL * (int64_t)rusage.ru_utime.tv_sec +
           (int64_t)rusage.ru_utime.tv_usec * 1000ULL;
      t += 1000000000ULL * (int64_t)rusage.ru_stime.tv_sec +
           (int64_t)rusage.ru_stime.tv_usec * 1000ULL;
    }
    return t;
#else
    return 0;
#endif
  }

  void reset() { total_ns = 0; }

  void begin() { total_ns -= query(); }

  void end() { total_ns += query(); }

  float sec() const { return total_ns * 1e-9f; }
#else
  static int64_t query() { return 0; }
  void reset() {}
  void begin() {}
  void end() {}
  float sec() const { return 0; }
#endif
};

#define MAX_REG_COUNT 1000
using namespace Shell;

bool echo_mode = false;
Command *first_queued_cmd;
Command *current_cmd;

std::map<std::string, Command *> cmd_register;

Command::Command(std::string name, std::string short_help)
    : cmd_name(name), short_help(short_help) {
  next_queued_cmd = first_queued_cmd;
  first_queued_cmd = this;
  call_counter = 0;
  runtime_ns = 0;
}

void Command::runRegister() {
  if (cmd_register.count(cmd_name)) {
    PLOGE.printf("Unable to register cmd '%s', cmd already exists!\n",
                 cmd_name.c_str());
  } else {
    cmd_register[cmd_name] = this;
  }
}

void Command::initRegister() {
  std::vector<Command *> added_cmdes;
  while (first_queued_cmd) {
    added_cmdes.push_back(first_queued_cmd);
    first_queued_cmd->runRegister();
    first_queued_cmd = first_queued_cmd->next_queued_cmd;
  }
  for (auto added_cmd : added_cmdes)
    added_cmd->onRegister();
}

void Command::doneRegister() {
  for (auto &it : cmd_register)
    it.second->onShutdown();

  cmd_register.clear();
}

void Command::onRegister() {}

void Command::onShutdown() {}

Command::~Command() {
  // 清理分配的内存
  for (auto &pair : _args) {
    delete pair.second;
  }
}

Command::pre_post_exec_state_t Command::preExecute() {
  pre_post_exec_state_t state;
  call_counter++;
  state.begin_ns = PerformanceTimer::query();
  state.parent_cmd = current_cmd;
  current_cmd = this;
  clearFlags();
  return state;
}

void Command::postExecute(Command::pre_post_exec_state_t state) {
  int64_t time_ns = PerformanceTimer::query() - state.begin_ns;
  runtime_ns += time_ns;
  current_cmd = state.parent_cmd;
  if (current_cmd)
    current_cmd->runtime_ns -= time_ns;
}

void Command::help() {
  PLOGI.printf("No help message for command `%s'.", cmd_name.c_str());
}

void Command::clearFlags() {}

void Command::cmdError(const std::vector<std::string> &args, size_t argidx,
                       std::string msg) {
  std::string command_text;
  int error_pos = 0;

  for (size_t i = 0; i < args.size(); i++) {
    if (i < argidx)
      error_pos += args[i].size() + 1;
    command_text = command_text + (command_text.empty() ? "" : " ") + args[i];
  }

  CmdLog::logInfo("Syntax error in command `%s':", command_text.c_str());
  help();
  CmdLog::logCmdError("Command syntax error: %s\n> %s\n> %*s^\n", msg.c_str(),
                      command_text.c_str(), error_pos, "");
}

void Command::runCommand(const std::string &command) {
  std::vector<std::string> args;

  call(args);
}

void Command::call(std::vector<std::string> args) {
  if (args.size() == 0 || args[0][0] == '#' || args[0][0] == ':')
    return;

  // change by lzx
  if (cmd_register.count(args[0]) == 0) {
    PLOGE.printf("No such command: %s (type 'help' for a command overview)",
                 args[0].c_str());
    return;
  }
  std::string cmd_str;
  for (auto &&str : args) {
    cmd_str += str + " ";
  }
  CmdLog::logInfo("Executing pass '%s'.", cmd_str.c_str());
  cmd_register[args[0]]->parse(args);
  auto state = cmd_register[args[0]]->preExecute();
  cmd_register[args[0]]->execute();
  cmd_register[args[0]]->postExecute(state);
}

// 向 Command 中添加字符串参数
void Command::addStringArgument(const std::string &key,
                                const std::string &short_help, int optional,
                                int group) {
  StringArgument *argu =
      new StringArgument(this, key, short_help, optional, group);
  _args[key] = argu;
  _groups[group].push_back(argu);
}

// 向 Command 中添加整数参数
void Command::addIntArgument(const std::string &key,
                             const std::string &short_help, int optional,
                             int group) {
  IntArgument *argu = new IntArgument(this, key, short_help, optional, group);
  _args[key] = argu;
  _groups[group].push_back(argu);
}

// 向 Command 中添加布尔参数
void Command::addBoolArgument(const std::string &key,
                              const std::string &short_help, int optional,
                              int group) {
  BoolArgument *argu = new BoolArgument(this, key, short_help, optional, group);
  _args[key] = argu;
  _groups[group].push_back(argu);
}

// 向 Command 中添加字符串数组参数
void Command::addStringArrayArgument(const std::string &key,
                                     const std::string &short_help,
                                     int optional, int group) {
  StringArrayArgument *argu =
      new StringArrayArgument(this, key, short_help, optional, group);
  _args[key] = argu;
  _groups[group].push_back(argu);
}

// 向 Pass 中添加Map参数
void Command::addMapArgument(const std::string &key,
                             const std::string &short_help, int optional,
                             int group) {
  MapArgument *argu = new MapArgument(this, key, short_help, optional, group);
  _args[key] = argu;
  _groups[group].push_back(argu);
}

bool Command::hasValue(const std::string &key) const {
  assert(_args.count(key));
  return _args.at(key)->isSet();
}

// 检查参数是否为字符串类型
bool Command::isString(const std::string &key) const {
  if (_args.count(key)) {
    return _args.at(key)->isString();
  }
  return false;
}

// 检查参数是否为整数类型
bool Command::isInt(const std::string &key) const {
  if (_args.count(key)) {
    return _args.at(key)->isInt();
  }
  return false;
}

// 检查参数是否为布尔类型
bool Command::isBool(const std::string &key) const {
  if (_args.count(key)) {
    return _args.at(key)->isBool();
  }
  return false;
}

// 检查参数是否为字符串数组类型
bool Command::isStringArray(const std::string &key) const {
  if (_args.count(key)) {
    return _args.at(key)->isStringArray();
  }
  return false;
}

// 检查参数是否为Map类型
bool Command::isMap(const std::string &key) const {
  if (_args.count(key)) {
    return _args.at(key)->isMap();
  }
  return false;
}

// 获取字符串参数的值
std::string Command::getStringValue(const std::string &key) const {
  if (_args.count(key)) {
    return dynamic_cast<StringArgument *>(_args.at(key))->getStringValue();
  }
  return "";
}

// 获取整数参数的值
int Command::getIntValue(const std::string &key) const {
  if (_args.count(key)) {
    return dynamic_cast<IntArgument *>(_args.at(key))->getIntValue();
  }
  throw std::invalid_argument("Invalid argument type or not found: " + key);
}

// 获取布尔参数的值
bool Command::getBoolValue(const std::string &key) const {
  if (_args.count(key)) {
    return dynamic_cast<BoolArgument *>(_args.at(key))->getBoolValue();
  }
  throw std::invalid_argument("Invalid argument type or not found: " + key);
}

// 获取字符串数组参数的值
std::vector<std::string>
Command::getStringArrayValue(const std::string &key) const {
  if (_args.count(key)) {
    return dynamic_cast<StringArrayArgument *>(_args.at(key))
        ->getStringArrayValue();
  }
  throw std::invalid_argument("Invalid argument type or not found: " + key);
}

// 获取Map参数的值
std::map<std::string, std::string>
Command::getMapValue(const std::string &key) const {
  if (_args.count(key)) {
    return dynamic_cast<MapArgument *>(_args.at(key))->getMapValue();
  }
  throw std::invalid_argument("Invalid argument type or not found: " + key);
}

Argument *Command::findArgument(const std::string &str) {
  Argument *result = nullptr;
  if (str.empty())
    return result;
  std::string key;
  if (str[0] == '-') {
    key = str.substr(1);
  } else {
    key = "";
  }
  return _args.count(key) ? _args.at(key) : nullptr;
}

// 解析命令字符串，填充参数值
void Command::parse(const std::vector<std::string> &tokens) {
  if (tokens.empty())
    return;

  for (int i = 1; i < (int)tokens.size(); ++i) {
    const std::string &token = tokens[i];
    Argument *arg = findArgument(token);
    if (arg) {
      arg->parseValue(i, tokens);
    } else {
      CmdLog::logCmdError("Invalid argument: %s", token.c_str());
      return;
    }
  }
}

SingleObjCmd::SingleObjCmd(std::string name, std::string short_help)
    : Command(name, short_help) {
  addStringArgument("design", "design name");
}

Command::pre_post_exec_state_t SingleObjCmd::preExecute() {
  if (hasValue("design")) {
    std::string design_name = getStringValue("design");
    DataBase::Design *design =
        DataBase::DesignSet::instance()->design(design_name);
    if (design == nullptr) {
      CmdLog::logCmdError("Can't find design '%s'", design_name.c_str());
    } else {
      _cur_design = design;
    }
  } else {
    DataBase::Design *design = DataBase::DesignSet::instance()->currentDesign(true);
    if (design == nullptr) {
      CmdLog::logCmdError("The current design is null.");
    } else {
      _cur_design = design;
    }
  }
  return Command::preExecute();
}
