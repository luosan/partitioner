#ifndef ARGUMENT_H
#define ARGUMENT_H

#include "register.h"
#include <map>
#include <string>
#include <vector>

namespace Shell {

struct Command;

// Argument class
class Argument {
public:
  Argument(Command *owner, const std::string &name,
           const std::string &short_help, int optional = 1, int group = 1)
      : _owner(owner), _name(name), _shortHelp(short_help), _optional(optional),
        _group(group), _set(false) {}

  virtual ~Argument() {}

  virtual bool isValidValue(int &i,
                            const std::vector<std::string> &tokens) const {
    return false;
  };
  virtual void parseValue(int &i, const std::vector<std::string> &tokens) = 0;
  virtual bool isString() const = 0;
  virtual bool isInt() const = 0;
  virtual bool isBool() const = 0;
  virtual bool isStringArray() const = 0;
  virtual bool isMap() const = 0;

  std::string getShortHelp() const { return _shortHelp; }

  int getGroup() const { return _group; }

  bool isSet() const { return _set; }

  int optional() const;
  void setOptional(int newOptional);

  std::string name() const;
  void setName(const std::string &newName);

  void setShortHelp(const std::string &newShortHelp);

  void setGroup(int newGroup);

  void setSet(bool newSet);

protected:
  Command *_owner;
  std::string _name;
  std::string _shortHelp;
  int _optional = 1;
  int _group = 1;
  bool _set = false;
};

// String argument class
class StringArgument : public Argument {
public:
  StringArgument(Command *owner, const std::string &name,
                 const std::string &short_help, int optional = 1,
                 int group = 1);

  bool isValidValue(int &i,
                    const std::vector<std::string> &tokens) const override;

  void parseValue(int &i, const std::vector<std::string> &tokens) override;

  bool isString() const override { return true; }

  bool isInt() const override { return false; }

  bool isBool() const override { return false; }

  bool isStringArray() const override { return false; }

  bool isMap() const override { return false; }

  std::string getStringValue() const { return _value; }

private:
  std::string _value;
};

// Integer argument class
class IntArgument : public Argument {
public:
  IntArgument(Command *owner, const std::string &name,
              const std::string &short_help, int optional = 1, int group = 1)
      : Argument(owner, name, short_help, optional, group) {}

  bool isValidValue(int &i,
                    const std::vector<std::string> &tokens) const override;

  void parseValue(int &i, const std::vector<std::string> &tokens) override;

  bool isString() const override { return false; }

  bool isInt() const override { return true; }

  bool isBool() const override { return false; }

  bool isStringArray() const override { return false; }

  bool isMap() const override { return false; }

  int getIntValue() const { return _value; }

private:
  int _value;
};

// Boolean argument class
class BoolArgument : public Argument {
public:
  BoolArgument(Command *owner, const std::string &name,
               const std::string &short_help, int optional = 1, int group = 1);

  bool isValidValue(int &i,
                    const std::vector<std::string> &tokens) const override;

  void parseValue(int &i, const std::vector<std::string> &tokens) override;

  bool isString() const override { return false; }

  bool isInt() const override { return false; }

  bool isBool() const override { return true; }

  bool isStringArray() const override { return false; }

  bool isMap() const override { return false; }

  bool getBoolValue() const { return _value; }

private:
  bool _value = false;
};

// String array argument class
class StringArrayArgument : public Argument {
public:
  StringArrayArgument(Command *owner, const std::string &name,
                      const std::string &short_help, int optional = 1,
                      int group = 1)
      : Argument(owner, name, short_help, optional, group) {}

  bool isValidValue(int &i,
                    const std::vector<std::string> &tokens) const override;

  void parseValue(int &i, const std::vector<std::string> &tokens) override;

  bool isString() const override { return false; }

  bool isInt() const override { return false; }

  bool isBool() const override { return false; }

  bool isStringArray() const override { return true; }

  bool isMap() const override { return false; }

  std::vector<std::string> getStringArrayValue() const { return _value; }

private:
  std::vector<std::string> _value;
};

// Map argument class
class MapArgument : public Argument {
public:
  MapArgument(Command *owner, const std::string &name,
              const std::string &short_help, int optional = 1, int group = 1)
      : Argument(owner, name, short_help, optional, group) {}

  bool isValidValue(int &i,
                    const std::vector<std::string> &tokens) const override;

  // 解析值为 { key value } 或 key value 形式的参数
  void parseValue(int &i, const std::vector<std::string> &tokens) override;

  bool isString() const override { return false; }

  bool isInt() const override { return false; }

  bool isBool() const override { return false; }

  bool isStringArray() const override { return false; }

  bool isMap() const override { return true; }

  std::map<std::string, std::string> getMapValue() const { return _value; }

private:
  std::map<std::string, std::string> _value;
};
} // namespace Shell
#endif // ARGUMENT_H
