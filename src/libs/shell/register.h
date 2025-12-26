#ifndef REGISTER_H
#define REGISTER_H

#include "cmdlog.h"
#include <map>
#include <string>
#include <vector>

namespace DataBase {
class Design;
}

namespace Shell {

class Argument;

struct Command {
  std::string cmd_name, short_help;
  std::map<std::string, Argument *> _args;
  std::map<Argument *, bool> _argSet;
  std::map<int, std::vector<Argument *>> _groups;

  Command(std::string name, std::string short_help = "** document me **");
  virtual ~Command();

  virtual void help();
  virtual void clearFlags();
  virtual void execute() = 0;

  void addStringArgument(const std::string &key,
                         const std::string &short_help = "", int optional = 1,
                         int group = 1);
  void addIntArgument(const std::string &key,
                      const std::string &short_help = "", int optional = 1,
                      int group = 1);
  void addBoolArgument(const std::string &key,
                       const std::string &short_help = "", int optional = 1,
                       int group = 1);
  void addStringArrayArgument(const std::string &key,
                              const std::string &short_help = "",
                              int optional = 1, int group = 1);
  void addMapArgument(const std::string &key,
                      const std::string &short_help = "", int optional = 1,
                      int group = 1);

  bool hasValue(const std::string &key) const;
  bool isString(const std::string &key) const;
  bool isInt(const std::string &key) const;
  bool isBool(const std::string &key) const;
  bool isStringArray(const std::string &key) const;
  bool isMap(const std::string &key) const;

  std::string getStringValue(const std::string &key) const;
  int getIntValue(const std::string &key) const;
  bool getBoolValue(const std::string &key) const;
  std::vector<std::string> getStringArrayValue(const std::string &key) const;
  std::map<std::string, std::string> getMapValue(const std::string &key) const;

  int call_counter;
  int64_t runtime_ns;

  struct pre_post_exec_state_t {
    Command *parent_cmd;
    int64_t begin_ns;
  };

  Argument *findArgument(const std::string &str);
  void parse(const std::vector<std::string> &command);

  virtual pre_post_exec_state_t preExecute();
  virtual void postExecute(pre_post_exec_state_t state);

  void cmdError(const std::vector<std::string> &args, size_t argidx,
                std::string msg);

  static void runCommand(const std::string &command);

  static void call(std::vector<std::string> args);

  Command *next_queued_cmd;
  virtual void runRegister();
  static void initRegister();
  static void doneRegister();

  virtual void onRegister();
  virtual void onShutdown();
};

struct SingleObjCmd : public Command {
  DataBase::Design *_cur_design = nullptr;
  SingleObjCmd(std::string name, std::string short_help = "** document me **");

  virtual pre_post_exec_state_t preExecute();
};

} // namespace Shell
extern std::map<std::string, Shell::Command *> cmd_register;

#endif
