#ifndef UTIL_H
#define UTIL_H

#include <string>

class Util
{
public:
    Util();
    static std::string next_token(std::string &text, const char *sep = " \t\r\n", bool long_strings = false);
};

#endif // UTIL_H
