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

namespace datagen::logging {

struct LoggerHolder {
    Logger instance;
};

LoggerHolder logger_holder;

std::string current_timestamp() {
    const auto     now         = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm         local_time{};
#if defined(_WIN32)
    localtime_s(&local_time, &now_time);
#else
    localtime_r(&now_time, &local_time);
#endif

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

Logger& Logger::instance() {
    return logger_holder.instance;
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
    error_emitted_ = false;
}

void Logger::disable_file_logging() {
    std::lock_guard lock(mutex_);
    generate_log_output_.reset();
    error_log_output_.reset();
    error_emitted_ = false;
}

bool Logger::consume_error_emitted() {
    std::lock_guard lock(mutex_);
    const bool emitted = error_emitted_;
    error_emitted_     = false;
    return emitted;
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
    if (static_cast<int>(level) < static_cast<int>(minimum_level_)) { return; }
    if (level == LogLevel::Error) { error_emitted_ = true; }
    const std::string line = "[" + current_timestamp() + "] " + message;

    if (echo_to_stderr_) { std::cerr << line << "\n"; }

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
    default             : return "INFO";
    }
}

std::string format_progress_bar(const std::uint64_t done, const std::uint64_t total) {
    constexpr int kWidth   = 20;
    const double  progress = total == 0 ? 1.0 : static_cast<double>(done) / static_cast<double>(total);
    const int     filled   = static_cast<int>(progress * static_cast<double>(kWidth));

    std::string bar;
    bar.reserve(kWidth);
    for (int i = 0; i < kWidth; ++i) { bar.push_back(i < filled ? '#' : '-'); }

    std::ostringstream oss;
    oss << "[" << bar << "] " << static_cast<int>(progress * 100.0) << "% (" << done << "/" << total << ")";
    return oss.str();
}

}  // namespace datagen::logging
