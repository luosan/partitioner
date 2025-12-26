// SPDX-License-Identifier: BSD-3-Clause
// Simple logger interface for standalone TritonPart

#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace par {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    
    void setLevel(LogLevel level) { level_ = level; }
    LogLevel getLevel() const { return level_; }
    
    void debug(const std::string& msg) {
        if (level_ <= LogLevel::DEBUG) {
            std::cout << "[DEBUG] " << msg << std::endl;
        }
    }
    
    void info(const std::string& msg) {
        if (level_ <= LogLevel::INFO) {
            std::cout << "[INFO] " << msg << std::endl;
        }
    }
    
    void warning(const std::string& msg) {
        if (level_ <= LogLevel::WARNING) {
            std::cerr << "[WARNING] " << msg << std::endl;
        }
    }
    
    void error(const std::string& msg) {
        if (level_ <= LogLevel::ERROR) {
            std::cerr << "[ERROR] " << msg << std::endl;
        }
    }
    
    void critical(const std::string& msg) {
        if (level_ <= LogLevel::CRITICAL) {
            std::cerr << "[CRITICAL] " << msg << std::endl;
        }
    }
    
    void report(const std::string& msg) {
        std::cout << msg << std::endl;
    }
    
    // Helper to convert value to string
    template<typename T>
    static std::string toString(const T& val) {
        std::ostringstream oss;
        oss << val;
        return oss.str();
    }
    
    // Format string with {} placeholders (simplified fmt-style)
    template<typename T, typename... Args>
    static std::string formatString(const std::string& fmt, const T& first, Args... rest) {
        std::string result = fmt;
        size_t pos = result.find("{}");
        if (pos == std::string::npos) {
            // Check for {:.2f} style
            pos = result.find("{:");
            if (pos != std::string::npos) {
                size_t end = result.find("}", pos);
                if (end != std::string::npos) {
                    result.replace(pos, end - pos + 1, toString(first));
                }
            }
        } else {
            result.replace(pos, 2, toString(first));
        }
        if constexpr (sizeof...(rest) > 0) {
            return formatString(result, rest...);
        }
        return result;
    }
    
    static std::string formatString(const std::string& fmt) {
        return fmt;
    }
    
    // Template version for compatibility with format-style calls
    template<typename... Args>
    void report(const char* format, Args... args) {
        std::cout << formatString(std::string(format), args...) << std::endl;
    }
    
    template<typename... Args>
    void info(const char* format, Args... args) {
        if (level_ <= LogLevel::INFO) {
            std::cout << "[INFO] " << formatString(std::string(format), args...) << std::endl;
        }
    }
    
    template<typename... Args>
    void error(const char* format, Args... args) {
        if (level_ <= LogLevel::ERROR) {
            std::cerr << "[ERROR] " << formatString(std::string(format), args...) << std::endl;
        }
    }
    
    template<typename... Args>
    void warning(const char* format, Args... args) {
        if (level_ <= LogLevel::WARNING) {
            std::cerr << "[WARNING] " << formatString(std::string(format), args...) << std::endl;
        }
    }
    
    // debugCheck for compatibility - always returns true in standalone version
    bool debugCheck(int module, const char* group, int level) {
        (void)module;
        (void)group;
        (void)level;
        return level_ <= LogLevel::DEBUG;
    }
    
private:
    Logger() : level_(LogLevel::INFO) {}
    LogLevel level_;
};

// Macro for compatibility
#define debugPrint(logger, module, level, ...) \
    do { \
        if (logger) { \
            std::ostringstream oss; \
            oss << __VA_ARGS__; \
            logger->debug(oss.str()); \
        } \
    } while(0)

} // namespace par

// Put utl namespace at global scope for compatibility
namespace utl {
    using Logger = par::Logger;
    
    // Minimal module enum to satisfy "using utl::PAR;" etc.
    enum Module {
        PAR = 0
    };
}
