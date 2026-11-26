// HB Core - Homebax Studios. Copyright (c) 2026 Ondrej Jurasik / Homebax Studios.
#pragma once
#include <functional>
#include <mutex>
#include <sstream>
#include <string>

namespace HB {

enum class LogLevel { Trace = 0, Debug = 1, Info = 2, Warning = 3, Error = 4, Off = 5 };

class Logger {
public:
    using Sink = std::function<void(LogLevel, const std::string&)>;

    static Logger& instance();

    void setLevel(LogLevel level);
    LogLevel level() const;
    // Replaces the sink. An empty function restores the default stderr sink.
    void setSink(Sink sink);
    void log(LogLevel level, const std::string& message);
    static const char* levelName(LogLevel level);

private:
    Logger() = default;
    mutable std::mutex mutex_;
    LogLevel level_ = LogLevel::Info;
    Sink sink_;
};

} // namespace HB

#define HB_LOG(lvl, expr)                                              \
    do {                                                               \
        if (::HB::Logger::instance().level() <= (lvl)) {               \
            std::ostringstream hb_log_stream_;                         \
            hb_log_stream_ << expr;                                    \
            ::HB::Logger::instance().log((lvl), hb_log_stream_.str()); \
        }                                                              \
    } while (0)

#define HB_LOG_DEBUG(expr) HB_LOG(::HB::LogLevel::Debug, expr)
#define HB_LOG_INFO(expr) HB_LOG(::HB::LogLevel::Info, expr)
#define HB_LOG_WARN(expr) HB_LOG(::HB::LogLevel::Warning, expr)
#define HB_LOG_ERROR(expr) HB_LOG(::HB::LogLevel::Error, expr)
