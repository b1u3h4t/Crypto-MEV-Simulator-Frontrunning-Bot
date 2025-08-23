#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

namespace mev {

// Log levels
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

// Logging configuration
struct LoggingConfig {
    LogLevel level = LogLevel::INFO;
    std::string file = "logs/mev_sim.log";
    uint32_t max_file_size_mb = 100;
    uint32_t max_files = 10;
    bool console_output = true;
    bool file_output = true;
    bool timestamp = true;
    bool thread_id = false;
};

// Logger class - thread-safe singleton
class Logger {
public:
    // Initialize logger with configuration
    static void initialize(const LoggingConfig& config);
    
    // Shutdown logger
    static void shutdown();
    
    // Logging methods
    static void trace(const std::string& message);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);
    static void fatal(const std::string& message);
    
    // Log with custom level
    static void log(LogLevel level, const std::string& message);
    
    // Set log level
    static void set_level(LogLevel level);
    
    // Get current log level
    static LogLevel get_level();
    
    // Check if level is enabled
    static bool is_enabled(LogLevel level);
    
    // Flush log buffer
    static void flush();

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Singleton instance
    static Logger& get_instance();
    
    // Internal logging method
    void log_internal(LogLevel level, const std::string& message);
    
    // Format message
    std::string format_message(LogLevel level, const std::string& message);
    
    // Get level string
    std::string get_level_string(LogLevel level);
    
    // Get timestamp string
    std::string get_timestamp();
    
    // Rotate log file if needed
    void rotate_log_file();
    
    // Write to file
    void write_to_file(const std::string& message);
    
    // Write to console
    void write_to_console(const std::string& message);
    
    // Member variables
    LoggingConfig config_;
    std::unique_ptr<std::ofstream> file_stream_;
    std::mutex mutex_;
    uint64_t current_file_size_ = 0;
    bool initialized_ = false;
};

// Convenience macros for logging
#define LOG_TRACE(msg) mev::Logger::trace(msg)
#define LOG_DEBUG(msg) mev::Logger::debug(msg)
#define LOG_INFO(msg) mev::Logger::info(msg)
#define LOG_WARN(msg) mev::Logger::warn(msg)
#define LOG_ERROR(msg) mev::Logger::error(msg)
#define LOG_FATAL(msg) mev::Logger::fatal(msg)

// Conditional logging macros
#define LOG_TRACE_IF(condition, msg) if (condition) mev::Logger::trace(msg)
#define LOG_DEBUG_IF(condition, msg) if (condition) mev::Logger::debug(msg)
#define LOG_INFO_IF(condition, msg) if (condition) mev::Logger::info(msg)
#define LOG_WARN_IF(condition, msg) if (condition) mev::Logger::warn(msg)
#define LOG_ERROR_IF(condition, msg) if (condition) mev::Logger::error(msg)
#define LOG_FATAL_IF(condition, msg) if (condition) mev::Logger::fatal(msg)

// Stream-based logging
class LogStream {
public:
    LogStream(LogLevel level) : level_(level) {}
    ~LogStream() {
        if (!message_.str().empty()) {
            Logger::log(level_, message_.str());
        }
    }
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        message_ << value;
        return *this;
    }
    
private:
    LogLevel level_;
    std::ostringstream message_;
};

// Stream logging macros
#define LOG_STREAM_TRACE mev::LogStream(mev::LogLevel::TRACE)
#define LOG_STREAM_DEBUG mev::LogStream(mev::LogLevel::DEBUG)
#define LOG_STREAM_INFO mev::LogStream(mev::LogLevel::INFO)
#define LOG_STREAM_WARN mev::LogStream(mev::LogLevel::WARN)
#define LOG_STREAM_ERROR mev::LogStream(mev::LogLevel::ERROR)
#define LOG_STREAM_FATAL mev::LogStream(mev::LogLevel::FATAL)

} // namespace mev
