// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#include "base/log.hpp"
#include <cstdio>

namespace HB {

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::level() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void Logger::setSink(Sink sink) {
    std::lock_guard<std::mutex> lock(mutex_);
    sink_ = std::move(sink);
}

const char* Logger::levelName(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Off: return "OFF";
    }
    return "?";
}

void Logger::log(LogLevel level, const std::string& message) {
    Sink sink;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (level < level_) return;
        sink = sink_;
    }
    if (sink) {
        sink(level, message);
    } else {
        std::fprintf(stderr, "[HB Core][%s] %s\n", levelName(level), message.c_str());
    }
}

} // namespace HB
