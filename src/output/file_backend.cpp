// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file file_backend.cpp

#include "output/file_backend.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "logging/logger.h"
#include "output/file/csv_writer.h"
#include "output/file/json_writer.h"
#include "output/file/sql_writer.h"

namespace data_generator::output {

namespace {

std::string now_compact_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto tt  = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d%H%M%S");
    return oss.str();
}

std::string extension_for_format(const config::OutputFormat format) {
    switch (format) {
    case config::OutputFormat::Csv         : return "csv";
    case config::OutputFormat::JsonFormat  : return "json";
    case config::OutputFormat::Sql         : return "sql";
    case config::OutputFormat::TabDelimited: return "tsv";
    default:
        return "txt";
    }
}

std::string build_default_output_path(const config::OutputFormat format) {
    return "dgresult_" + now_compact_timestamp() + "." + extension_for_format(format);
}

std::uint64_t estimate_row_size(const config::GenerationConfig& cfg) {
    const auto sample = engine::preview_row(cfg);
    std::uint64_t size = 0;
    for (const auto& value : sample) {
        if (value.has_value()) {
            size += value->size();
        } else {
            size += 4;
        }
    }

    size += static_cast<std::uint64_t>(cfg.fields.size() * 3);
    return std::max<std::uint64_t>(size, 1);
}

bool prompt_large_output_if_needed(
    const std::uint64_t estimated_total_bytes,
    const std::uint64_t threshold_bytes
) {
    if (estimated_total_bytes <= threshold_bytes) { return true; }

    std::cout << "Estimated output size is " << estimated_total_bytes << " bytes (threshold " << threshold_bytes
              << "). Continue? [y/N]: ";
    std::cout.flush();

    std::string answer;
    if (!std::getline(std::cin, answer)) { return false; }
    return answer == "y" || answer == "Y" || answer == "yes" || answer == "YES";
}

}  // namespace

OutputStats FileBackend::generate(const config::GenerationConfig& cfg, const engine::ExecutionOptions& options) {
    logging::Logger& logger = logging::Logger::instance();

    const config::OutputFormat format = cfg.output.file.format;
    const std::filesystem::path output_path = cfg.output.file.path.empty()
                                                  ? std::filesystem::path(build_default_output_path(format))
                                                  : std::filesystem::path(cfg.output.file.path);

    const std::uint64_t estimated_row_size = estimate_row_size(cfg);
    const std::uint64_t estimated_total_size = estimated_row_size * static_cast<std::uint64_t>(cfg.rows);
    logger.info(
        "Estimated row size=" + std::to_string(estimated_row_size) +
        " bytes, estimated total file size=" + std::to_string(estimated_total_size) + " bytes"
    );

    constexpr std::uint64_t kLargeOutputThresholdBytes = 128ULL * 1024ULL * 1024ULL;
    if (!prompt_large_output_if_needed(estimated_total_size, kLargeOutputThresholdBytes)) {
        throw std::runtime_error("generation cancelled by user");
    }

    std::ofstream output_stream(output_path, std::ios::trunc);
    if (!output_stream) {
        throw std::runtime_error("failed to open output file: " + output_path.string());
    }

    logger.info("Output backend=file path=" + output_path.string());

    std::vector<std::string> columns;
    columns.reserve(cfg.fields.size());
    for (const auto& field : cfg.fields) { columns.push_back(field.name); }

    output::file::DelimitedWriterOptions delimited_options;
    if (format == config::OutputFormat::Csv) {
        delimited_options.delimiter = ",";
        delimited_options.quote = "\"";
        delimited_options.header = cfg.output.file.csv.header;
        delimited_options.line_ending = cfg.output.file.csv.line_ending;
        output::file::write_delimited_header(columns, output_stream, delimited_options);
    } else if (format == config::OutputFormat::TabDelimited) {
        delimited_options.delimiter = "\t";
        delimited_options.quote = "\"";
        delimited_options.header = cfg.output.file.tab_delimited.header;
        delimited_options.line_ending = cfg.output.file.tab_delimited.line_ending;
        output::file::write_delimited_header(columns, output_stream, delimited_options);
    } else if (format == config::OutputFormat::Custom) {
        delimited_options.delimiter = cfg.output.file.custom.delimiter;
        delimited_options.quote = cfg.output.file.custom.quote;
        delimited_options.header = cfg.output.file.custom.header;
        delimited_options.line_ending = cfg.output.file.custom.line_ending;
        output::file::write_delimited_header(columns, output_stream, delimited_options);
    } else if (format == config::OutputFormat::JsonFormat) {
        if (cfg.output.file.json.array) {
            output::file::write_json_array_start(output_stream);
        }
    } else if (format == config::OutputFormat::Sql) {
        output::file::write_sql(columns, {}, cfg.output.file.sql.table, cfg.output.file.sql.create_table, output_stream);
    }

    std::uint64_t generated = 0;
    std::uint64_t last_printed = 0;
    bool progress_printed = false;
    const auto started_at = std::chrono::steady_clock::now();

    const engine::GenerateResult result = engine::generate_with_consumer(
        cfg,
        options,
        [&](engine::Row&& row, std::uint64_t) {
            if (format == config::OutputFormat::Csv ||
                format == config::OutputFormat::TabDelimited ||
                format == config::OutputFormat::Custom) {
                output::file::write_delimited_row(row, output_stream, delimited_options);
            } else if (format == config::OutputFormat::JsonFormat) {
                output::file::write_json_row(columns, row, output_stream, generated == 0, cfg.output.file.json);
            } else {
                output::file::write_sql_row(columns, row, cfg.output.file.sql.table, output_stream);
            }

            ++generated;
            if (generated % 2000 == 0) {
                std::cout << "\r[Rows Generated] "
                          << logging::format_progress_bar(generated, static_cast<std::uint64_t>(cfg.rows))
                          << std::flush;
                progress_printed = true;
                last_printed = generated;
            }
            return true;
        }
    );

    if (!progress_printed || last_printed != generated) {
        std::cout << "\r[Rows Generated] "
                  << logging::format_progress_bar(generated, static_cast<std::uint64_t>(cfg.rows));
    }
    std::cout << "\n" << std::flush;

    if (format == config::OutputFormat::JsonFormat && cfg.output.file.json.array) {
        output::file::write_json_array_end(output_stream);
    }

    output_stream.flush();
    if (!output_stream) {
        throw std::runtime_error("failed to flush output file: " + output_path.string());
    }

    const auto ended_at = std::chrono::steady_clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(ended_at - started_at).count();
    const double elapsed_sec = std::max(0.001, static_cast<double>(elapsed_ms) / 1000.0);
    const auto rate = static_cast<std::uint64_t>(static_cast<double>(generated) / elapsed_sec);
    logger.info("Generated rows=" + std::to_string(generated) + " rate=" + std::to_string(rate) + " rows/s");

    OutputStats stats;
    stats.execution_info = result.info;
    stats.rows_generated = generated;
    stats.rows_written = generated;
    return stats;
}

}  // namespace data_generator::output
