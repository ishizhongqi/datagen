// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file logger.h

#ifndef DATAGEN_LOGGER_H
#define DATAGEN_LOGGER_H

#include <cstdint>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>

namespace datagen::logging {

enum class LogLevel {
    Info,
    Warn,
    Error,
};

class Logger {
public:
    static Logger& instance();

    void set_minimum_level(LogLevel level);
    void set_echo_to_stderr(bool enabled);
    void configure_generate_logs(const std::string& generate_log_path, const std::string& error_log_path);
    void disable_file_logging();
    bool consume_error_emitted();

    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

private:
    Logger() = default;

    friend struct LoggerHolder;

    void write(LogLevel level, const std::string& message);

    LogLevel                     minimum_level_  = LogLevel::Info;
    bool                         echo_to_stderr_ = true;
    std::mutex                   mutex_;
    std::optional<std::ofstream> generate_log_output_;
    std::optional<std::ofstream> error_log_output_;
    bool                         error_emitted_ = false;
};

std::string log_level_to_string(LogLevel level);
std::string format_progress_bar(std::uint64_t done, std::uint64_t total);

}  // namespace datagen::logging

#endif
