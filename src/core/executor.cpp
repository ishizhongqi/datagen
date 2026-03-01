// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file executor.cpp

#include "core/executor.h"

#include <algorithm>
#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_set>

#include "core/serialization.h"
#include "generators/business_generators.h"
#include "generators/computer_generators.h"
#include "generators/datetime_generators.h"
#include "generators/location_generators.h"
#include "generators/number_generators.h"
#include "generators/payment_generators.h"
#include "generators/person_generators.h"
#include "generators/product_generators.h"
#include "generators/string_generators.h"
#include "generators/utility_generators.h"

namespace data_generator::core {

namespace {

using GeneratorPtr = std::unique_ptr<generator::IGenerator>;
using RuntimeField = GeneratorPtr;

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

std::vector<RuntimeField> build_runtime_fields(const GenerationConfig& cfg, const int rows_for_generator) {
    const generator::GeneratorRegistry registry = build_registry();
    std::vector<RuntimeField>          fields;
    fields.reserve(cfg.fields.size());

    for (const auto& field : cfg.fields) {
        Json field_cfg    = field.raw;
        field_cfg["rows"] = rows_for_generator;
        if (cfg.null_policy.configured) {
            if (cfg.null_policy.null_if_empty) {
                field_cfg["null_value_string"] = nullptr;
            } else if (cfg.null_policy.null_literal.has_value()) {
                field_cfg["null_value_string"] = *cfg.null_policy.null_literal;
            }
        }
        auto generator = registry.create(field.generator, field_cfg);
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
    static const std::unordered_set<std::string> kEligible = {
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
    return kEligible.contains(name);
}

bool can_parallelize_safely(const GenerationConfig& cfg, std::string& reason) {
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

Row materialize_row(std::vector<RuntimeField>* runtime_fields, const NullPolicy& policy) {
    Row row;
    row.reserve(runtime_fields->size());
    for (const auto& generator : *runtime_fields) {
        const std::string value = generator->generate();
        if (should_render_null(policy, value)) {
            row.emplace_back(std::nullopt);
        } else if (policy.configured && policy.null_literal.has_value() && value.empty()) {
            row.emplace_back(*policy.null_literal);
        } else {
            row.emplace_back(value);
        }
    }
    for (auto& generator : *runtime_fields) { generator->next(); }
    return row;
}

GenerateResult generate_sequential(
    const GenerationConfig& cfg,
    const RowConsumer&      consumer,
    std::atomic<bool>*      cancelled
) {
    auto runtime_fields = build_runtime_fields(cfg, cfg.rows);

    GenerateResult result;
    result.info.threads_used = 1;

    for (int i = 0; i < cfg.rows; ++i) {
        if (cancelled->load()) { break; }
        Row row = materialize_row(&runtime_fields, cfg.null_policy);
        if (!consumer(std::move(row), static_cast<std::uint64_t>(i))) {
            cancelled->store(true);
            break;
        }
        ++result.rows_generated;
    }

    return result;
}

GenerateResult generate_parallel(
    const GenerationConfig& cfg,
    const std::size_t       threads,
    const RowConsumer&      consumer,
    std::atomic<bool>*      cancelled
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
                auto       runtime_fields = build_runtime_fields(cfg, count);
                const auto row_offset     = static_cast<std::uint64_t>(begin);
                for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(count); ++i) {
                    if (cancelled->load()) { break; }
                    Row row = materialize_row(&runtime_fields, cfg.null_policy);
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

bool should_render_null(const NullPolicy& policy, const std::string& value) {
    return policy.configured && policy.null_if_empty && value.empty();
}

GenerateResult generate_with_consumer(
    const GenerationConfig& cfg,
    const ExecutionOptions& opts,
    const RowConsumer&      consumer
) {
    if (!consumer) { throw std::runtime_error("consumer callback is required"); }

    const std::size_t requested_threads = std::max<std::size_t>(1, opts.requested_threads);
    const std::size_t candidate_threads = effective_thread_count(requested_threads, cfg.rows);

    std::atomic<bool> cancelled = false;

    if (candidate_threads > 1) {
        std::string reason;
        if (can_parallelize_safely(cfg, reason)) {
            return generate_parallel(cfg, candidate_threads, consumer, &cancelled);
        }
        GenerateResult result = generate_sequential(cfg, consumer, &cancelled);
        result.info.fallback_to_single_thread = true;
        result.info.fallback_reason = std::move(reason);
        return result;
    }

    return generate_sequential(cfg, consumer, &cancelled);
}

GenerateResult generate_to_stream(const GenerationConfig& cfg, const ExecutionOptions& opts, std::ostream& out) {
    std::vector<Row> rows(static_cast<std::size_t>(cfg.rows));

    const GenerateResult result = generate_with_consumer(cfg, opts, [&](Row&& row, const std::uint64_t row_index) {
        rows[static_cast<std::size_t>(row_index)] = std::move(row);
        return true;
    });

    std::vector<std::string> columns;
    columns.reserve(cfg.fields.size());
    for (const auto& field : cfg.fields) { columns.push_back(field.name); }

    switch (cfg.format) {
    case OutputFormat::Csv : write_csv(columns, rows, out); break;
    case OutputFormat::Json: write_json(columns, rows, out); break;
    case OutputFormat::Sql : write_sql(columns, rows, cfg.table_name, out); break;
    }

    return result;
}

std::vector<std::optional<std::string>> preview_row(const GenerationConfig& cfg) {
    auto runtime_fields = build_runtime_fields(cfg, 1);
    auto row            = materialize_row(&runtime_fields, cfg.null_policy);
    return row;
}

}  // namespace data_generator::core
