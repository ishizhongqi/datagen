// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file executor.cpp

#include "engine/executor.h"

#include <algorithm>
#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_set>

#include "generators/core/override_rules.h"
#include "generators/types/business_generators.h"
#include "generators/types/computer_generators.h"
#include "generators/types/datetime_generators.h"
#include "generators/types/location_generators.h"
#include "generators/types/number_generators.h"
#include "generators/types/payment_generators.h"
#include "generators/types/person_generators.h"
#include "generators/types/product_generators.h"
#include "generators/types/string_generators.h"
#include "generators/types/utility_generators.h"
#include "output/file/csv_writer.h"
#include "output/file/json_writer.h"
#include "output/file/sql_writer.h"

namespace data_generator::engine {

namespace {

using GeneratorPtr = std::unique_ptr<generator::IGenerator>;
using RuntimeField = GeneratorPtr;
using Json         = config::Json;

const std::unordered_set<std::string> kParallelEligibleGenerators = {
    "boolean",
    "integer",
    "decimal",
    "date",
    "time",
    "datetime",
    "text",
    "uuid",
    "enum_item",
    "regular_expression",
};

std::size_t effective_thread_count(const std::size_t requested, const int rows) {
    if (rows <= 1) { return 1; }
    const std::size_t hw     = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    const std::size_t wanted = std::max<std::size_t>(1, requested);
    return std::min<std::size_t>({wanted, hw, static_cast<std::size_t>(rows)});
}

generator::GeneratorRegistry build_registry() {
    generator::GeneratorRegistry registry;
    register_computer_generators(registry);
    register_business_generators(registry);
    register_datetime_generators(registry);
    register_location_generators(registry);
    register_number_generators(registry);
    register_payment_generators(registry);
    register_person_generators(registry);
    register_product_generators(registry);
    register_string_generators(registry);
    register_utility_generators(registry);
    return registry;
}

std::vector<RuntimeField> build_runtime_fields(const config::GenerationConfig& cfg, const int rows_for_generator) {
    const generator::GeneratorRegistry registry = build_registry();
    std::vector<RuntimeField>          fields;
    fields.reserve(cfg.fields.size());

    for (const auto& field : cfg.fields) {
        Json field_cfg    = field.raw;
        field_cfg["rows"] = rows_for_generator;
        auto generator    = registry.create(field.generator, field_cfg);
        fields.emplace_back(std::move(generator));
    }

    return fields;
}

bool has_enabled_override(const Json& field, const std::string& key) {
    if (!field.contains(key) || !field.at(key).is_object()) { return false; }
    const auto& value = field.at(key);
    if (!value.contains("enabled")) { return false; }
    return value.at("enabled").is_boolean() && value.at("enabled").get<bool>();
}

bool is_parallel_eligible_generator(const std::string& name) {
    return kParallelEligibleGenerators.contains(name);
}

bool can_parallelize_safely(const config::GenerationConfig& cfg, std::string& reason) {
    for (size_t i = 0; i < cfg.fields.size(); ++i) {
        const auto&       field = cfg.fields[i];
        const std::string id    = "fields[" + std::to_string(i) + "]";
        if (!is_parallel_eligible_generator(field.generator)) {
            reason = id +
                     " uses generator '" +
                     field.generator +
                     "' which is not marked thread-safe for parallel row partitioning";
            return false;
        }
        if (field.unique) {
            reason = id + " enables unique output, which is global-state sensitive";
            return false;
        }
        if (field.data_linkage.has_value()) {
            reason = id + " uses data_linkage, which requires cross-field row state";
            return false;
        }
        if (has_enabled_override(field.raw, "null_value") || has_enabled_override(field.raw, "default_value")) {
            reason = id + " has row-order dependent override rules";
            return false;
        }
    }
    return true;
}

Row materialize_row(const std::vector<RuntimeField>* runtime_fields) {
    Row row;
    row.reserve(runtime_fields->size());
    for (const auto& generator : *runtime_fields) {
        const std::string value = generator->generate();
        if (value == generator::kNullSentinel) {
            row.emplace_back(std::nullopt);
        } else {
            row.emplace_back(value);
        }
    }
    for (auto& generator : *runtime_fields) { generator->next(); }
    return row;
}

GenerateResult generate_sequential(
    const config::GenerationConfig& cfg,
    const RowConsumer&              consumer,
    std::atomic<bool>*              cancelled
) {
    const auto runtime_fields = build_runtime_fields(cfg, cfg.rows);

    GenerateResult result;
    result.info.threads_used = 1;

    for (int i = 0; i < cfg.rows; ++i) {
        if (cancelled->load()) { break; }
        Row row = materialize_row(&runtime_fields);
        if (!consumer(std::move(row), static_cast<std::uint64_t>(i))) {
            cancelled->store(true);
            break;
        }
        ++result.rows_generated;
    }

    return result;
}

GenerateResult generate_parallel(
    const config::GenerationConfig& cfg,
    const std::size_t               threads,
    const RowConsumer&              consumer,
    std::atomic<bool>*              cancelled
) {
    GenerateResult result;
    result.info.threads_used = threads;

    std::vector<std::thread> workers;
    workers.reserve(threads);

    std::exception_ptr worker_exception;
    std::mutex         exception_mutex;

    std::atomic<std::uint64_t> generated_count = 0;

    const int base  = cfg.rows / static_cast<int>(threads);
    const int rem   = cfg.rows % static_cast<int>(threads);
    int       start = 0;

    for (std::size_t worker_index = 0; worker_index < threads; ++worker_index) {
        const int count  = base + (static_cast<int>(worker_index) < rem ? 1 : 0);
        const int begin  = start;
        start           += count;

        workers.emplace_back([&, begin, count] {
            try {
                const auto runtime_fields = build_runtime_fields(cfg, count);
                const auto row_offset     = static_cast<std::uint64_t>(begin);
                for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(count); ++i) {
                    if (cancelled->load()) { break; }
                    Row row = materialize_row(&runtime_fields);
                    if (!consumer(std::move(row), row_offset + i)) {
                        cancelled->store(true);
                        break;
                    }
                    ++generated_count;
                }
            } catch (...) {
                std::lock_guard lock(exception_mutex);
                if (!worker_exception) {
                    worker_exception = std::current_exception();
                    cancelled->store(true);
                }
            }
        });
    }

    for (auto& worker : workers) { worker.join(); }

    if (worker_exception) { std::rethrow_exception(worker_exception); }

    result.rows_generated = generated_count.load();
    return result;
}

}  // namespace

GenerateResult generate_with_consumer(
    const config::GenerationConfig& cfg,
    const ExecutionOptions&         opts,
    const RowConsumer&              consumer
) {
    if (!consumer) { throw std::runtime_error("consumer callback is required"); }

    const std::size_t requested_threads = std::max<std::size_t>(1, opts.requested_threads);
    const std::size_t candidate_threads = effective_thread_count(requested_threads, cfg.rows);

    std::atomic cancelled = false;

    if (candidate_threads > 1) {
        std::string reason;
        if (can_parallelize_safely(cfg, reason)) {
            return generate_parallel(cfg, candidate_threads, consumer, &cancelled);
        }
        GenerateResult result                 = generate_sequential(cfg, consumer, &cancelled);
        result.info.fallback_to_single_thread = true;
        result.info.fallback_reason           = std::move(reason);
        return result;
    }

    return generate_sequential(cfg, consumer, &cancelled);
}

GenerateResult
    generate_to_stream(const config::GenerationConfig& cfg, const ExecutionOptions& opts, std::ostream& out) {
    std::vector<Row> rows(static_cast<std::size_t>(cfg.rows));

    const GenerateResult result = generate_with_consumer(cfg, opts, [&](Row&& row, const std::uint64_t row_index) {
        rows[static_cast<std::size_t>(row_index)] = std::move(row);
        return true;
    });

    std::vector<std::string> columns;
    columns.reserve(cfg.fields.size());
    for (const auto& field : cfg.fields) { columns.push_back(field.name); }

    std::vector<bool> boolean_columns;
    boolean_columns.reserve(cfg.fields.size());
    for (const auto& field : cfg.fields) { boolean_columns.push_back(field.generator == "boolean"); }

    const config::OutputFormat           format = cfg.output.file.format;
    output::file::DelimitedWriterOptions delimited_options;
    switch (format) {
    case config::OutputFormat::Csv:
        delimited_options.delimiter   = ",";
        delimited_options.quote       = "\"";
        delimited_options.header      = cfg.output.file.csv.header;
        delimited_options.line_ending = cfg.output.file.csv.line_ending;
        output::file::write_delimited(columns, rows, out, delimited_options);
        break;
    case config::OutputFormat::TabDelimited:
        delimited_options.delimiter   = "\t";
        delimited_options.quote       = "\"";
        delimited_options.header      = cfg.output.file.tab_delimited.header;
        delimited_options.line_ending = cfg.output.file.tab_delimited.line_ending;
        output::file::write_delimited(columns, rows, out, delimited_options);
        break;
    case config::OutputFormat::Custom:
        delimited_options.delimiter   = cfg.output.file.custom.delimiter;
        delimited_options.quote       = cfg.output.file.custom.quote;
        delimited_options.header      = cfg.output.file.custom.header;
        delimited_options.line_ending = cfg.output.file.custom.line_ending;
        output::file::write_delimited(columns, rows, out, delimited_options);
        break;
    case config::OutputFormat::JsonFormat:
        output::file::write_json(columns, boolean_columns, rows, out, cfg.output.file.json);
        break;
    case config::OutputFormat::Sql:
        output::file::write_sql(
            columns,
            boolean_columns,
            rows,
            cfg.output.file.sql.table,
            cfg.output.file.sql.create_table,
            out
        );
        break;
    }

    return result;
}

std::vector<std::optional<std::string>> preview_row(const config::GenerationConfig& cfg) {
    const auto runtime_fields = build_runtime_fields(cfg, 1);
    auto       row            = materialize_row(&runtime_fields);
    return row;
}

}  // namespace data_generator::engine
