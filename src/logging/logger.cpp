// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file logger.cpp

#include "logging/logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace data_generator::logging {

namespace {

std::string now_string() {
    const auto now = std::chrono::system_clock::now();
    const auto tt  = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool should_log(const LogLevel level, const LogLevel minimum_level) {
    return static_cast<int>(level) >= static_cast<int>(minimum_level);
}

}  // namespace

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::set_minimum_level(const LogLevel level) {
    std::lock_guard lock(mutex_);
    minimum_level_ = level;
}

void Logger::set_echo_to_stderr(const bool enabled) {
    std::lock_guard lock(mutex_);
    echo_to_stderr_ = enabled;
}

void Logger::configure_generate_logs(const std::string& generate_log_path, const std::string& error_log_path) {
    std::lock_guard lock(mutex_);
    generate_log_output_.emplace(generate_log_path, std::ios::trunc);
    error_log_output_.emplace(error_log_path, std::ios::trunc);
}

void Logger::disable_file_logging() {
    std::lock_guard lock(mutex_);
    generate_log_output_.reset();
    error_log_output_.reset();
}

void Logger::info(const std::string& message) {
    write(LogLevel::Info, message);
}

void Logger::warn(const std::string& message) {
    write(LogLevel::Warn, message);
}

void Logger::error(const std::string& message) {
    write(LogLevel::Error, message);
}

void Logger::write(const LogLevel level, const std::string& message) {
    std::lock_guard lock(mutex_);
    if (!should_log(level, minimum_level_)) { return; }

    const std::string line = "[" + now_string() + "] [" + log_level_to_string(level) + "] " + message;

    if (echo_to_stderr_) {
        std::cerr << line << "\n";
    }

    if (generate_log_output_.has_value() && generate_log_output_->is_open()) {
        *generate_log_output_ << line << "\n";
    }

    if (level == LogLevel::Error && error_log_output_.has_value() && error_log_output_->is_open()) {
        *error_log_output_ << line << "\n";
    }
}

std::string log_level_to_string(const LogLevel level) {
    switch (level) {
    case LogLevel::Info : return "INFO";
    case LogLevel::Warn : return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

}  // namespace data_generator::logging
