#include "Logger.hpp"
#include <iostream>
#include <filesystem>
#include <thread>
#include <sstream>
#include <iomanip>

namespace mev {

Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const LoggingConfig& config) {
    auto& instance = get_instance();
    std::lock_guard<std::mutex> lock(instance.mutex_);
    
    if (instance.initialized_) {
        return; // Already initialized
    }
    
    instance.config_ = config;
    
    // Create logs directory if it doesn't exist
    if (config.file_output && !config.file.empty()) {
        std::filesystem::path log_path(config.file);
        std::filesystem::path log_dir = log_path.parent_path();
        if (!log_dir.empty() && !std::filesystem::exists(log_dir)) {
            std::filesystem::create_directories(log_dir);
        }
        
        // Open log file
        instance.file_stream_ = std::make_unique<std::ofstream>();
        instance.file_stream_->open(config.file, std::ios::app);
        
        if (!instance.file_stream_->is_open()) {
            throw std::runtime_error("Failed to open log file: " + config.file);
        }
        
        // Get current file size
        instance.file_stream_->seekp(0, std::ios::end);
        instance.current_file_size_ = instance.file_stream_->tellp();
    }
    
    instance.initialized_ = true;
    
    // Log initialization
    instance.log_internal(LogLevel::INFO, "Logger initialized");
}

void Logger::shutdown() {
    auto& instance = get_instance();
    std::lock_guard<std::mutex> lock(instance.mutex_);
    
    if (instance.initialized_) {
        instance.log_internal(LogLevel::INFO, "Logger shutting down");
        instance.flush();
        
        if (instance.file_stream_ && instance.file_stream_->is_open()) {
            instance.file_stream_->close();
        }
        
        instance.initialized_ = false;
    }
}

void Logger::trace(const std::string& message) {
    log(LogLevel::TRACE, message);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    auto& instance = get_instance();
    if (instance.initialized_ && level >= instance.config_.level) {
        instance.log_internal(level, message);
    }
}

void Logger::set_level(LogLevel level) {
    auto& instance = get_instance();
    std::lock_guard<std::mutex> lock(instance.mutex_);
    instance.config_.level = level;
}

LogLevel Logger::get_level() {
    auto& instance = get_instance();
    std::lock_guard<std::mutex> lock(instance.mutex_);
    return instance.config_.level;
}

bool Logger::is_enabled(LogLevel level) {
    auto& instance = get_instance();
    std::lock_guard<std::mutex> lock(instance.mutex_);
    return instance.initialized_ && level >= instance.config_.level;
}

void Logger::flush() {
    auto& instance = get_instance();
    std::lock_guard<std::mutex> lock(instance.mutex_);
    
    if (instance.file_stream_ && instance.file_stream_->is_open()) {
        instance.file_stream_->flush();
    }
}

void Logger::log_internal(LogLevel level, const std::string& message) {
    std::string formatted_message = format_message(level, message);
    
    if (config_.console_output) {
        write_to_console(formatted_message);
    }
    
    if (config_.file_output && file_stream_ && file_stream_->is_open()) {
        write_to_file(formatted_message);
    }
}

std::string Logger::format_message(LogLevel level, const std::string& message) {
    std::ostringstream oss;
    
    if (config_.timestamp) {
        oss << get_timestamp() << " ";
    }
    
    oss << "[" << get_level_string(level) << "] ";
    
    if (config_.thread_id) {
        oss << "[TID:" << std::this_thread::get_id() << "] ";
    }
    
    oss << message;
    
    return oss.str();
}

std::string Logger::get_level_string(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

void Logger::rotate_log_file() {
    if (!file_stream_ || !file_stream_->is_open()) {
        return;
    }
    
    // Check if file size exceeds limit
    uint64_t max_size_bytes = config_.max_file_size_mb * 1024 * 1024;
    if (current_file_size_ < max_size_bytes) {
        return;
    }
    
    // Close current file
    file_stream_->close();
    
    // Rotate existing log files
    std::filesystem::path log_path(config_.file);
    for (int i = config_.max_files - 1; i > 0; --i) {
        std::filesystem::path old_file = log_path.parent_path() / 
            (log_path.stem().string() + "." + std::to_string(i) + log_path.extension().string());
        std::filesystem::path new_file = log_path.parent_path() / 
            (log_path.stem().string() + "." + std::to_string(i + 1) + log_path.extension().string());
        
        if (std::filesystem::exists(old_file)) {
            if (i == config_.max_files - 1) {
                std::filesystem::remove(old_file);
            } else {
                std::filesystem::rename(old_file, new_file);
            }
        }
    }
    
    // Rename current log file to .1
    std::filesystem::path backup_file = log_path.parent_path() / 
        (log_path.stem().string() + ".1" + log_path.extension().string());
    std::filesystem::rename(log_path, backup_file);
    
    // Open new log file
    file_stream_->open(config_.file, std::ios::app);
    if (!file_stream_->is_open()) {
        throw std::runtime_error("Failed to open new log file after rotation");
    }
    
    current_file_size_ = 0;
}

void Logger::write_to_file(const std::string& message) {
    // Check if rotation is needed
    rotate_log_file();
    
    // Write message
    *file_stream_ << message << std::endl;
    current_file_size_ += message.length() + 1; // +1 for newline
    
    // Flush periodically
    static uint64_t flush_counter = 0;
    if (++flush_counter % 100 == 0) {
        file_stream_->flush();
    }
}

void Logger::write_to_console(const std::string& message) {
    // Use different colors for different log levels
    // This is a simple implementation - in production you might want more sophisticated coloring
    std::cout << message << std::endl;
}

} // namespace mev
