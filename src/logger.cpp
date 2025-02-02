#include "logger.hpp"
#include <stdexcept>

Logger::Logger(const std::string& log_dir) {
    std::string log_path = log_dir + "/processor.log";
    log_file.open(log_path, std::ios::out | std::ios::app);
    if (!log_file.is_open()) {
        throw std::runtime_error("Could not open log file: " + log_path);
    }
}

void Logger::log(Level level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    
    log_file << ss.str() << " [" << level_strings[level] << "] " 
            << message << std::endl;
    log_file.flush();
}

Logger::~Logger() {
    if (log_file.is_open()) {
        log_file.close();
    }
}