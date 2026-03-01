// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

#include "output/file_backend.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "core/serialization.h"
#include "core/workspace.h"
#include "logging/logger.h"

namespace data_generator::output {

namespace {

std::string format_progress_bar(const std::uint64_t done, const std::uint64_t total) {
    constexpr int kWidth = 20;
    const double progress = (total == 0) ? 1.0 : static_cast<double>(done) / static_cast<double>(total);
    const int filled = static_cast<int>(progress * static_cast<double>(kWidth));

    std::string bar;
    bar.reserve(kWidth);
    for (int i = 0; i < kWidth; ++i) {
        bar.push_back(i < filled ? '#' : '-');
    }

    std::ostringstream oss;
    oss << "[" << bar << "] " << static_cast<int>(progress * 100.0) << "% (" << done << "/" << total << ")";
    return oss.str();
}

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

std::string extension_for_format(const core::OutputFormat format) {
    switch (format) {
    case core::OutputFormat::Csv : return "csv";
    case core::OutputFormat::Json: return "json";
    case core::OutputFormat::Sql : return "sql";
    }
    return "txt";
}

std::string build_default_output_path(const core::OutputFormat format) {
    return "result_" + now_compact_timestamp() + "." + extension_for_format(format);
}

std::uint64_t estimate_row_size(const core::GenerationConfig& cfg) {
    const auto sample = core::preview_row(cfg);
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

void write_csv_row(const core::Row& row, std::ostream& out) {
    for (size_t i = 0; i < row.size(); ++i) {
        if (i > 0) { out << ","; }
        out << core::csv_escape(row[i].value_or(""));
    }
    out << "\n";
}

void write_json_row(const std::vector<std::string>& columns, const core::Row& row, std::ostream& out, const bool first) {
    if (!first) { out << ",\n"; }
    out << "  {";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) { out << ", "; }
        out << "\"" << columns[i] << "\":";
        if (i >= row.size() || !row[i].has_value()) {
            out << "null";
        } else {
            out << "\"" << core::sql_escape(*row[i]) << "\"";
        }
    }
    out << "}";
}

void write_sql_row(const std::vector<std::string>& columns, const core::Row& row, const std::string& table_name, std::ostream& out) {
    out << "INSERT INTO " << table_name << " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) { out << ", "; }
        out << columns[i];
    }
    out << ") VALUES (";
    for (size_t i = 0; i < row.size(); ++i) {
        if (i > 0) { out << ", "; }
        if (!row[i].has_value()) {
            out << "NULL";
        } else {
            out << "'" << core::sql_escape(*row[i]) << "'";
        }
    }
    out << ");\n";
}

}  // namespace

OutputStats FileBackend::generate(const core::GenerationConfig& cfg, const core::ExecutionOptions& options) {
    logging::Logger& logger = logging::Logger::instance();

    const std::string output_name = cfg.output.file.path.empty() ? build_default_output_path(cfg.format)
                                                                  : cfg.output.file.path;
    if (!core::is_workspace_local_filename(output_name)) {
        throw std::runtime_error("output filename must not include directory, file is stored under workspace root");
    }

    const std::filesystem::path output_path = std::filesystem::path(cfg.workspace) / output_name;

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

    if (cfg.format == core::OutputFormat::Csv) {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) { output_stream << ","; }
            output_stream << core::csv_escape(columns[i]);
        }
        output_stream << "\n";
    } else if (cfg.format == core::OutputFormat::Json) {
        output_stream << "[\n";
    }

    std::uint64_t generated = 0;
    const auto started_at = std::chrono::steady_clock::now();

    const core::GenerateResult result = core::generate_with_consumer(cfg, options, [&](core::Row&& row, std::uint64_t) {
        if (cfg.format == core::OutputFormat::Csv) {
            write_csv_row(row, output_stream);
        } else if (cfg.format == core::OutputFormat::Json) {
            write_json_row(columns, row, output_stream, generated == 0);
        } else {
            write_sql_row(columns, row, cfg.table_name, output_stream);
        }

        ++generated;
        if (generated % 2000 == 0 || generated == static_cast<std::uint64_t>(cfg.rows)) {
            std::cerr << "\r[Rows Generated] " << format_progress_bar(generated, static_cast<std::uint64_t>(cfg.rows))
                      << "\n[Data Imported ] " << format_progress_bar(generated, static_cast<std::uint64_t>(cfg.rows))
                      << std::flush;
        }
        return true;
    });

    if (cfg.format == core::OutputFormat::Json) {
        output_stream << "\n]\n";
    }

    output_stream.flush();
    if (!output_stream) {
        throw std::runtime_error("failed to flush output file: " + output_path.string());
    }

    const auto ended_at = std::chrono::steady_clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(ended_at - started_at).count();
    const double elapsed_sec = std::max(0.001, static_cast<double>(elapsed_ms) / 1000.0);
    const std::uint64_t rate = static_cast<std::uint64_t>(static_cast<double>(generated) / elapsed_sec);
    logger.info("Generated rows=" + std::to_string(generated) + " rate=" + std::to_string(rate) + " rows/s");

    OutputStats stats;
    stats.execution_info = result.info;
    stats.rows_generated = generated;
    stats.rows_written = generated;
    return stats;
}

}  // namespace data_generator::output
