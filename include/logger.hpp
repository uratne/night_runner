#pragma once
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>

class Logger {
private:
    std::ofstream log_file;
    const char* level_strings[4] = {"DEBUG", "INFO", "WARNING", "ERROR"};

public:
    enum Level {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };

    Logger(const std::string& log_dir);
    void log(Level level, const std::string& message);
    ~Logger();
};