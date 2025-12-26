#include "argument.h"
#include "cmdlog.h"
#include <cassert>
#include <tcl.h>

#if TCL_MAJOR_VERSION == 8 && TCL_MINOR_VERSION < 7
using Tcl_Size = int;
#endif

using namespace Shell;

int Argument::optional() const { return _optional; }

void Argument::setOptional(int newOptional) { _optional = newOptional; }

std::string Argument::name() const { return _name; }

void Argument::setName(const std::string &newName) { _name = newName; }

void Argument::setShortHelp(const std::string &newShortHelp) {
  _shortHelp = newShortHelp;
}

void Argument::setGroup(int newGroup) { _group = newGroup; }

void Argument::setSet(bool newSet) { _set = newSet; }

StringArgument::StringArgument(Command *owner, const std::string &name,
                               const std::string &short_help, int optional,
                               int group)
    : Argument(owner, name, short_help, optional, group) {}

bool StringArgument::isValidValue(
    int &i, const std::vector<std::string> &tokens) const {
  if (_name.empty()) {
    if (i >= (int)tokens.size()) {
      return false;
    }
    std::string value = tokens[i];
    if (value.empty() || value[0] == '-') {
      return false;
    }
    return true;
  } else {
    if (i + 1 >= (int)tokens.size()) {
      return false;
    }
    std::string value = tokens[i + 1];
    if (value.empty() || value[0] == '-') {
      return false;
    }
    return true;
  }
}

inline void StringArgument::parseValue(int &i,
                                       const std::vector<std::string> &tokens) {
  if (isValidValue(i, tokens)) {
    std::string value = _name.empty() ? tokens[i] : tokens[i + 1];

    _set = true;
    _owner->_argSet[this] = true;
    _value = value;
    if (!_name.empty()) {
      ++i;
    }
  } else {
    CmdLog::logCmdError("Invalid value for argument: '%s'", _name.c_str());
    return;
  }
}

bool IntArgument::isValidValue(int &i,
                               const std::vector<std::string> &tokens) const {
  if (_name.empty()) {
    if (i >= (int)tokens.size()) {
      return false;
    }
    std::string value = tokens[i];
    if (value.empty() || value[0] == '-') {
      return false;
    }
    try {
      std::stoi(value); // 尝试将值转换为整数
      return true;
    } catch (...) {
      return false; // 转换失败，值无效
    }
  } else {
    if (i + 1 >= (int)tokens.size()) {
      return false;
    }
    std::string value = tokens[i + 1];
    if (value.empty() || value[0] == '-') {
      return false;
    }
    try {
      std::stoi(value); // 尝试将值转换为整数
      return true;
    } catch (...) {
      return false; // 转换失败，值无效
    }
  }
}

void IntArgument::parseValue(int &i, const std::vector<std::string> &tokens) {
  if (isValidValue(i, tokens)) {
    std::string value = _name.empty() ? tokens[i] : tokens[i + 1];
    _set = true;
    _value = std::stoi(value);
    _owner->_argSet[this] = true;
    if (!_name.empty()) {
      ++i;
    }
  } else {
    CmdLog::logCmdError("Invalid value for argument: '%s'", _name.c_str());
    return;
  }
}

BoolArgument::BoolArgument(Command *owner, const std::string &name,
                           const std::string &short_help, int optional,
                           int group)
    : Argument(owner, name, short_help, optional, group) {
  assert(!name.empty());
}

bool BoolArgument::isValidValue(int &i,
                                const std::vector<std::string> &tokens) const {
  if (i < (int)tokens.size() && !tokens[i].empty() && tokens[i][0] == '-')
    return true;
  else
    return false;
}

void BoolArgument::parseValue(int &i, const std::vector<std::string> &tokens) {
  _owner->_argSet[this] = true;
  _set = true;
}

bool StringArrayArgument::isValidValue(
    int &i, const std::vector<std::string> &tokens) const {
  if (_name.empty()) {
    if (i >= (int)tokens.size()) {
      return false;
    }
    return true;
  } else {
    if (i + 1 >= (int)tokens.size()) {
      return false;
    }
    return true;
  }
}

void StringArrayArgument::parseValue(int &i,
                                     const std::vector<std::string> &tokens) {
  if (!isValidValue(i, tokens)) {
    CmdLog::logCmdError("Invalid value for argument: '%s'", _name.c_str());
    return;
  }
  std::string value = _name.empty() ? tokens[i] : tokens[i + 1];

  Tcl_Size objc;
  const char **objv = nullptr;
  Tcl_Interp *tcl_interp = Tcl_CreateInterp();
  if (Tcl_SplitList(tcl_interp, value.c_str(), &objc, &objv) != TCL_OK) {
    CmdLog::logCmdError("Invalid value for argument: '%s'", _name.c_str());
    return;
  }

  _value.clear();
  for (Tcl_Size j = 0; j < objc; ++j) {
    _value.emplace_back(objv[j]);
  }

  Tcl_Free(const_cast<char *>(reinterpret_cast<const char *>(objv)));

  _set = true;
  _owner->_argSet[this] = true;
  if (!_name.empty()) {
    ++i;
  }
}

bool MapArgument::isValidValue(int &i,
                               const std::vector<std::string> &tokens) const {
  return true;
}

void MapArgument::parseValue(int &i, const std::vector<std::string> &tokens) {
  std::string value = _name.empty() ? tokens[i] : tokens[i + 1];
  std::string key;
  _value.clear();
  if (_name.empty()) {
    key = tokens[i];
    value = tokens[i + 1];
    i = i + 1;
    _value[key] = value;
  } else {
    key = tokens[i + 1];
    value = tokens[i + 2];
    i = i + 2;
    _value[key] = value;
  }
  _owner->_argSet[this] = true;
  _set = true;
}
