// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file database_backend.cpp

#include "output/database_backend.h"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#include "database/db_schema_validator.h"
#include "database/db_url_parser.h"
#include "database/driver/idatabase_driver.h"
#include "database/type_adapter.h"
#include "logging/logger.h"

namespace data_generator::output {

namespace {

using RowWithIndex = std::pair<std::uint64_t, core::Row>;

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

core::InsertMode choose_insert_mode(
    const core::GenerationConfig&    cfg,
    const database::IDatabaseDriver& driver
) {
    if (cfg.output.database.insert_mode != core::InsertMode::Auto) {
        return cfg.output.database.insert_mode;
    }

    if (driver.supports_load_mode() && cfg.rows >= 50000) { return core::InsertMode::Load; }
    if (cfg.output.database.batch_size > 1) { return core::InsertMode::Bulk; }
    return core::InsertMode::Insert;
}

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(std::max<std::size_t>(1, capacity)) {}

    bool push(T value, const std::atomic<bool>* stop_flag) {
        std::unique_lock lock(mutex_);
        can_push_.wait(lock, [&] {
            return closed_ || queue_.size() < capacity_ || (stop_flag && stop_flag->load());
        });
        if (closed_ || (stop_flag && stop_flag->load())) { return false; }

        queue_.push_back(std::move(value));
        can_pop_.notify_one();
        return true;
    }

    bool pop(T* out, const std::atomic<bool>* stop_flag) {
        std::unique_lock lock(mutex_);
        can_pop_.wait(lock, [&] {
            return closed_ || !queue_.empty() || (stop_flag && stop_flag->load());
        });

        if (queue_.empty()) { return false; }

        *out = std::move(queue_.front());
        queue_.pop_front();
        can_push_.notify_one();
        return true;
    }

    void close() {
        std::lock_guard lock(mutex_);
        closed_ = true;
        can_push_.notify_all();
        can_pop_.notify_all();
    }

private:
    std::size_t              capacity_;
    std::deque<T>            queue_;
    bool                     closed_ = false;
    std::mutex               mutex_;
    std::condition_variable  can_push_;
    std::condition_variable  can_pop_;
};

std::string join_columns(const std::vector<std::string>& columns) {
    std::ostringstream oss;
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) { oss << ", "; }
        oss << columns[i];
    }
    return oss.str();
}

std::string quote_identifier_segment(const database::DbType db_type, const std::string& value) {
    const char quote = (db_type == database::DbType::Mysql) ? '`' : '"';
    std::string escaped;
    escaped.reserve(value.size() + 2);
    for (const char ch : value) {
        escaped.push_back(ch);
        if (ch == quote) { escaped.push_back(ch); }
    }
    return std::string(1, quote) + escaped + std::string(1, quote);
}

std::string quote_identifier_path(const database::DbType db_type, const std::string& value) {
    std::ostringstream oss;
    std::size_t        begin = 0;
    bool               first = true;
    while (begin <= value.size()) {
        const std::size_t end = value.find('.', begin);
        const std::string part = end == std::string::npos ? value.substr(begin) : value.substr(begin, end - begin);
        if (!first) { oss << "."; }
        oss << quote_identifier_segment(db_type, part);
        if (end == std::string::npos) { break; }
        begin = end + 1;
        first = false;
    }
    return oss.str();
}

std::string sql_escape_single_quote(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const char ch : value) {
        escaped.push_back(ch);
        if (ch == '\'') { escaped.push_back('\''); }
    }
    return escaped;
}

std::string escape_for_load_file(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
        case '\\': escaped += "\\\\"; break;
        case '\t': escaped += "\\t"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        default  : escaped.push_back(ch); break;
        }
    }
    return escaped;
}

std::string render_row_for_load(const std::vector<std::string>& sql_literals) {
    std::ostringstream oss;
    for (size_t i = 0; i < sql_literals.size(); ++i) {
        if (i > 0) { oss << '\t'; }
        if (sql_literals[i] == "NULL") {
            oss << "\\N";
        } else if (sql_literals[i].size() >= 2 && sql_literals[i].front() == '\'' && sql_literals[i].back() == '\'') {
            const std::string value = sql_literals[i].substr(1, sql_literals[i].size() - 2);
            oss << escape_for_load_file(value);
        } else {
            oss << escape_for_load_file(sql_literals[i]);
        }
    }
    oss << "\n";
    return oss.str();
}

std::string build_insert_sql(
    const database::DbType db_type,
    const std::string& table_name,
    const std::vector<std::string>& columns,
    const std::vector<std::vector<std::string>>& rows,
    const core::InsertMode mode
) {
    std::ostringstream oss;

    if (mode == core::InsertMode::Insert) {
        if (rows.empty()) { return ""; }
        oss << "INSERT INTO " << table_name << " (" << join_columns(columns) << ") VALUES (";
        for (size_t i = 0; i < rows[0].size(); ++i) {
            if (i > 0) { oss << ", "; }
            oss << rows[0][i];
        }
        oss << ")";
        return oss.str();
    }

    if (db_type == database::DbType::Oracle) {
        oss << "INSERT ALL ";
        for (const auto& row_values : rows) {
            oss << "INTO " << table_name << " (" << join_columns(columns) << ") VALUES (";
            for (size_t col_index = 0; col_index < row_values.size(); ++col_index) {
                if (col_index > 0) { oss << ", "; }
                oss << row_values[col_index];
            }
            oss << ") ";
        }
        oss << "SELECT 1 FROM DUAL";
        return oss.str();
    }

    oss << "INSERT INTO " << table_name << " (" << join_columns(columns) << ") VALUES ";
    for (size_t row_index = 0; row_index < rows.size(); ++row_index) {
        if (row_index > 0) { oss << ", "; }
        oss << "(";
        for (size_t col_index = 0; col_index < rows[row_index].size(); ++col_index) {
            if (col_index > 0) { oss << ", "; }
            oss << rows[row_index][col_index];
        }
        oss << ")";
    }

    return oss.str();
}

std::string build_load_sql(
    const database::DbType db_type,
    const std::string& table_name,
    const std::vector<std::string>& columns,
    const std::string& path
) {
    std::ostringstream oss;
    if (db_type == database::DbType::Mysql) {
        oss << "LOAD DATA LOCAL INFILE '" << sql_escape_single_quote(path) << "' INTO TABLE " << table_name
            << " FIELDS TERMINATED BY '\\t' ESCAPED BY '\\\\' LINES TERMINATED BY '\\n' ("
            << join_columns(columns) << ")";
        return oss.str();
    }
    if (db_type == database::DbType::Postgresql) {
        oss << "COPY " << table_name << " (" << join_columns(columns) << ") FROM '"
            << sql_escape_single_quote(path)
            << "' WITH (FORMAT text, DELIMITER E'\\t', NULL '\\\\N')";
        return oss.str();
    }
    return "";
}

bool begin_transaction(
    const database::DbType db_type,
    database::IDatabaseDriver* driver,
    std::string* error_message
) {
    if (!driver) {
        if (error_message) { *error_message = "driver pointer is null"; }
        return false;
    }
    if (db_type == database::DbType::Oracle) { return true; }
    return driver->execute("START TRANSACTION", error_message);
}

bool commit_transaction(database::IDatabaseDriver* driver, std::string* error_message) {
    if (!driver) {
        if (error_message) { *error_message = "driver pointer is null"; }
        return false;
    }
    return driver->execute("COMMIT", error_message);
}

void rollback_transaction(database::IDatabaseDriver* driver, std::string* error_message) {
    if (!driver) { return; }
    (void)driver->execute("ROLLBACK", error_message);
}

bool supports_multi_row_values(const database::DbType db_type) {
    return db_type != database::DbType::Oracle;
}

std::string build_oracle_temporal_literal(
    const database::ColumnTypeFamily family,
    const std::string& converted_value
) {
    if (family == database::ColumnTypeFamily::DateTime) {
        return "TO_TIMESTAMP('" + sql_escape_single_quote(converted_value) + "', 'YYYY-MM-DD HH24:MI:SS')";
    }
    if (family == database::ColumnTypeFamily::Date) {
        return "TO_DATE('" + sql_escape_single_quote(converted_value) + "', 'YYYY-MM-DD')";
    }
    if (family == database::ColumnTypeFamily::Time) {
        return "TO_TIMESTAMP('" + sql_escape_single_quote(converted_value) + "', 'HH24:MI:SS')";
    }
    return "'" + sql_escape_single_quote(converted_value) + "'";
}

std::string truncate_for_log(const std::string& text, const std::size_t max_size) {
    if (text.size() <= max_size) { return text; }
    return text.substr(0, max_size) + "...<truncated>";
}

bool is_load_data_disabled_error(const std::string& error_text) {
    const std::string lower = [&] {
        std::string text = error_text;
        std::transform(text.begin(), text.end(), text.begin(), [](const unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return text;
    }();
    return lower.find("local data is disabled") != std::string::npos ||
           lower.find("local_infile") != std::string::npos;
}

}  // namespace

OutputStats DatabaseBackend::generate(const core::GenerationConfig& cfg, const core::ExecutionOptions& options) {
    logging::Logger& logger = logging::Logger::instance();

    database::DbUrl db_url;
    std::string     error;
    if (!database::parse_db_url(cfg.output.database.url, &db_url, &error)) {
        throw std::runtime_error("invalid database URL: " + error);
    }

    auto driver = database::make_database_driver(db_url.type);
    if (!driver) {
        throw std::runtime_error("unsupported database type: " + database::db_type_to_string(db_url.type));
    }
    if (!driver->connect(db_url, &error)) {
        throw std::runtime_error("database connection failed: " + error);
    }

    const std::string table_name = cfg.output.database.table_name.empty() ? cfg.table_name
                                                                           : cfg.output.database.table_name;

    database::TableMetadata metadata;
    if (!driver->get_table_metadata(table_name, &metadata, &error)) {
        throw std::runtime_error("failed to load table metadata: " + error);
    }

    logger.info("Database type=" + database::db_type_to_string(db_url.type));
    logger.info("Insert mode=" + core::insert_mode_to_string(cfg.output.database.insert_mode));
    logger.info("Batch size=" + std::to_string(cfg.output.database.batch_size));
    logger.info("Queue size=" + std::to_string(cfg.output.database.queue_size));
    logger.info("DB threads=" + std::to_string(cfg.output.database.db_threads));

    const database::SchemaValidationReport report = database::validate_table_schema(cfg, metadata);
    for (const auto& message : report.messages) {
        const std::string text = "[schema] " + message.message;
        if (message.level == database::ValidationLevel::Error) {
            logger.error(text);
        } else if (message.level == database::ValidationLevel::Warn) {
            logger.warn(text);
        } else {
            logger.info(text);
        }
    }
    if (report.error_count() > 0) {
        throw std::runtime_error("table schema validation failed");
    }

    const core::InsertMode resolved_mode = choose_insert_mode(cfg, *driver);
    logger.info("Resolved insert mode=" + core::insert_mode_to_string(resolved_mode));

    std::unordered_map<std::string, const database::ColumnMetadata*> column_map;
    for (const auto& column : metadata.columns) {
        column_map[column.name] = &column;
    }

    std::vector<std::string> columns;
    columns.reserve(cfg.fields.size());
    std::vector<std::string> sql_columns;
    sql_columns.reserve(cfg.fields.size());
    std::vector<const database::ColumnMetadata*> mapped_columns;
    mapped_columns.reserve(cfg.fields.size());
    for (const auto& field : cfg.fields) {
        if (!column_map.contains(field.name)) {
            throw std::runtime_error("column not found in metadata: " + field.name);
        }
        columns.push_back(field.name);
        sql_columns.push_back(quote_identifier_path(db_url.type, field.name));
        mapped_columns.push_back(column_map.at(field.name));
    }
    const std::string sql_table_name = quote_identifier_path(db_url.type, table_name);

    const int total_rows = cfg.rows;
    const int batch_size = std::max(1, cfg.output.database.batch_size);
    const int queue_size = std::max(1, cfg.output.database.queue_size);
    int db_threads = std::max(1, cfg.output.database.db_threads);

    if (cfg.output.database.transaction_mode == core::TransactionMode::PerRun && db_threads > 1) {
        logger.warn("transaction_mode=per-run only supports db_threads=1, forcing db_threads=1");
        db_threads = 1;
    }

    std::atomic<bool> stop{false};
    std::atomic<bool> rollback_all{false};
    std::atomic<bool> load_fallback_warned{false};
    std::atomic<std::uint64_t> rows_generated{0};
    std::atomic<std::uint64_t> rows_imported{0};
    std::vector<std::filesystem::path> temp_files;
    std::mutex temp_files_mutex;

    BoundedQueue<RowWithIndex> queue(static_cast<std::size_t>(queue_size));

    std::mutex error_mutex;
    std::string first_error;

    auto register_error = [&](const std::string& text) {
        std::lock_guard lock(error_mutex);
        if (first_error.empty()) { first_error = text; }
        logger.error(text);
    };

    auto handle_policy = [&](const core::ErrorPolicy policy, const std::string& context_error) {
        if (policy == core::ErrorPolicy::Continue) {
            logger.warn(context_error + " (continue)");
            return;
        }
        if (policy == core::ErrorPolicy::RollbackBatch) {
            logger.warn(context_error + " (rollback current batch)");
            return;
        }
        if (policy == core::ErrorPolicy::RollbackAll) {
            rollback_all.store(true);
        }
        stop.store(true);
    };

    std::atomic<bool> progress_stop{false};
    std::thread progress_thread([&] {
        bool printed = false;
        while (!progress_stop.load()) {
            const std::uint64_t generated = rows_generated.load();
            const std::uint64_t imported = rows_imported.load();
            if (printed) { std::cerr << "\033[2F"; }
            std::cerr << "[Rows Generated] " << format_progress_bar(generated, static_cast<std::uint64_t>(total_rows))
                      << "\n"
                      << "[Data Imported ] " << format_progress_bar(imported, static_cast<std::uint64_t>(total_rows))
                      << "\n"
                      << std::flush;
            printed = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    const auto started_at = std::chrono::steady_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(db_threads));

    for (int worker_id = 0; worker_id < db_threads; ++worker_id) {
        workers.emplace_back([&, worker_id] {
            auto worker_driver = database::make_database_driver(db_url.type);
            if (!worker_driver) {
                register_error("failed to create database driver for worker");
                stop.store(true);
                return;
            }
            std::string worker_error;
            if (!worker_driver->connect(db_url, &worker_error)) {
                register_error("worker connection failed: " + worker_error);
                stop.store(true);
                return;
            }

            const database::DefaultTypeAdapter adapter;

            const bool per_run_tx = (cfg.output.database.transaction_mode == core::TransactionMode::PerRun);
            if (per_run_tx) {
                if (!begin_transaction(db_url.type, worker_driver.get(), &worker_error)) {
                    register_error("worker failed to start transaction: " + worker_error);
                    stop.store(true);
                    return;
                }
            }

            std::vector<RowWithIndex> batch;
            batch.reserve(static_cast<std::size_t>(batch_size));

            auto process_batch = [&](const std::vector<RowWithIndex>& rows) {
                if (rows.empty()) { return true; }

                std::vector<std::vector<std::string>> sql_rows;
                sql_rows.reserve(rows.size());

                for (const auto& [row_index, row] : rows) {
                    std::vector<std::string> sql_values;
                    sql_values.reserve(mapped_columns.size());

                    for (size_t i = 0; i < mapped_columns.size(); ++i) {
                        const auto adapted = adapter.adapt(*mapped_columns[i], row[i]);
                        if (!adapted.ok) {
                            std::ostringstream oss;
                            oss << "row=" << row_index
                                << " field=" << cfg.fields[i].name
                                << " raw=" << (row[i].has_value() ? *row[i] : "<NULL>")
                                << " converted=" << adapted.converted_value
                                << " error_type=" << adapted.error_type
                                << " message=" << adapted.error_message;
                            register_error(oss.str());
                            handle_policy(cfg.output.database.error_policy, "type conversion failed");
                            return cfg.output.database.error_policy == core::ErrorPolicy::Continue ||
                                   cfg.output.database.error_policy == core::ErrorPolicy::RollbackBatch;
                        }
                        std::string sql_literal = adapted.sql_literal;
                        if (db_url.type == database::DbType::Oracle && !adapted.is_null) {
                            const database::ColumnTypeFamily family = database::classify_column_type(*mapped_columns[i]);
                            if (family == database::ColumnTypeFamily::DateTime ||
                                family == database::ColumnTypeFamily::Date ||
                                family == database::ColumnTypeFamily::Time) {
                                sql_literal = build_oracle_temporal_literal(family, adapted.converted_value);
                            }
                        }
                        sql_values.push_back(std::move(sql_literal));
                    }

                    sql_rows.push_back(std::move(sql_values));
                }

                auto execute_sql = [&](const std::string& sql) {
                    if (sql.empty()) { return true; }
                    if (!worker_driver->execute(sql, &worker_error)) {
                        register_error(
                            "sql execution failed: " + worker_error +
                            " sql=" + truncate_for_log(sql, 4096)
                        );
                        handle_policy(cfg.output.database.error_policy, "sql execution failed");
                        return false;
                    }
                    return true;
                };

                const bool per_batch_tx = (cfg.output.database.transaction_mode == core::TransactionMode::PerBatch);
                if (per_batch_tx) {
                    if (!begin_transaction(db_url.type, worker_driver.get(), &worker_error)) {
                        register_error("failed to start batch transaction: " + worker_error);
                        stop.store(true);
                        return false;
                    }
                }

                bool ok = true;
                if (resolved_mode == core::InsertMode::Insert) {
                    for (const auto& row_values : sql_rows) {
                        if (!execute_sql(build_insert_sql(
                                db_url.type,
                                sql_table_name,
                                sql_columns,
                                {row_values},
                                core::InsertMode::Insert
                            ))) {
                            ok = false;
                            break;
                        }
                    }
                } else if (resolved_mode == core::InsertMode::Bulk) {
                    if (!supports_multi_row_values(db_url.type)) {
                        for (const auto& row_values : sql_rows) {
                            if (!execute_sql(build_insert_sql(
                                    db_url.type,
                                    sql_table_name,
                                    sql_columns,
                                    {row_values},
                                    core::InsertMode::Insert
                                ))) {
                                ok = false;
                                break;
                            }
                        }
                    } else {
                        ok = execute_sql(build_insert_sql(
                            db_url.type,
                            sql_table_name,
                            sql_columns,
                            sql_rows,
                            core::InsertMode::Bulk
                        ));
                    }
                } else {
                    const std::filesystem::path load_file =
                        std::filesystem::path(cfg.workspace) / "tmp" /
                        ("load_" + std::to_string(worker_id) + "_" + std::to_string(rows.front().first) + ".tmp");
                    std::ofstream temp(load_file, std::ios::trunc);
                    if (!temp) {
                        register_error("failed to open temp load file: " + load_file.string());
                        stop.store(true);
                        ok = false;
                    } else {
                        for (const auto& row_values : sql_rows) {
                            temp << render_row_for_load(row_values);
                        }
                        temp.close();
                        {
                            std::lock_guard lock(temp_files_mutex);
                            temp_files.push_back(load_file);
                        }
                        const std::string load_sql = build_load_sql(db_url.type, sql_table_name, sql_columns, load_file.string());
                        if (load_sql.empty()) {
                            ok = false;
                            register_error("load mode is not supported for current database type");
                            handle_policy(cfg.output.database.error_policy, "load mode not supported");
                        } else if (!worker_driver->execute(load_sql, &worker_error)) {
                            const bool can_fallback_to_bulk =
                                db_url.type == database::DbType::Mysql && is_load_data_disabled_error(worker_error);
                            if (can_fallback_to_bulk) {
                                bool expected = false;
                                if (load_fallback_warned.compare_exchange_strong(expected, true)) {
                                    logger.warn(
                                        "LOAD DATA LOCAL INFILE is disabled, fallback to bulk insert"
                                    );
                                }
                                ok = execute_sql(build_insert_sql(
                                    db_url.type,
                                    sql_table_name,
                                    sql_columns,
                                    sql_rows,
                                    core::InsertMode::Bulk
                                ));
                            } else {
                                register_error(
                                    "sql execution failed: " + worker_error +
                                    " sql=" + truncate_for_log(load_sql, 4096)
                                );
                                handle_policy(cfg.output.database.error_policy, "sql execution failed");
                                ok = false;
                            }
                        } else {
                            ok = true;
                        }
                    }
                }

                if (per_batch_tx) {
                    if (ok) {
                        if (!commit_transaction(worker_driver.get(), &worker_error)) {
                            register_error("commit failed: " + worker_error);
                            stop.store(true);
                            return false;
                        }
                    } else {
                        rollback_transaction(worker_driver.get(), &worker_error);
                    }
                }

                if (ok) {
                    rows_imported.fetch_add(static_cast<std::uint64_t>(rows.size()));

                    const int rate_limit = cfg.output.database.rate_limit_rows_per_sec;
                    if (rate_limit > 0) {
                        const double sec = static_cast<double>(rows.size()) / static_cast<double>(rate_limit);
                        const auto wait_ms = static_cast<int>(sec * 1000.0);
                        if (wait_ms > 0) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
                        }
                    }
                }

                return ok;
            };

            while (!stop.load()) {
                RowWithIndex item;
                if (!queue.pop(&item, &stop)) { break; }
                batch.push_back(std::move(item));

                if (static_cast<int>(batch.size()) >= batch_size) {
                    const bool ok = process_batch(batch);
                    batch.clear();
                    if (!ok && cfg.output.database.error_policy == core::ErrorPolicy::Stop) { break; }
                    if (stop.load()) { break; }
                }
            }

            if (!batch.empty() && !stop.load()) {
                (void)process_batch(batch);
            }

            if (per_run_tx) {
                if (rollback_all.load() || stop.load()) {
                    rollback_transaction(worker_driver.get(), &worker_error);
                } else if (!commit_transaction(worker_driver.get(), &worker_error)) {
                    register_error("per-run commit failed: " + worker_error);
                    stop.store(true);
                }
            }

            worker_driver->disconnect();
        });
    }

    core::GenerateResult generate_result;
    try {
        generate_result = core::generate_with_consumer(cfg, options, [&](core::Row&& row, std::uint64_t row_index) {
            if (stop.load()) { return false; }
            const bool pushed = queue.push(RowWithIndex{row_index, std::move(row)}, &stop);
            if (pushed) { rows_generated.fetch_add(1); }
            return pushed;
        });
    } catch (const std::exception& ex) {
        register_error(std::string("generator failed: ") + ex.what());
        stop.store(true);
    }

    queue.close();
    for (auto& worker : workers) { worker.join(); }

    progress_stop.store(true);
    if (progress_thread.joinable()) { progress_thread.join(); }

    for (const auto& path : temp_files) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    const auto ended_at = std::chrono::steady_clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(ended_at - started_at).count();
    const double elapsed_sec = std::max(0.001, static_cast<double>(elapsed_ms) / 1000.0);

    const std::uint64_t generated = rows_generated.load();
    const std::uint64_t imported = rows_imported.load();

    logger.info("Generated rows=" + std::to_string(generated));
    logger.info(
        "Generated rate=" +
        std::to_string(static_cast<std::uint64_t>(static_cast<double>(generated) / elapsed_sec)) +
        " rows/s"
    );
    logger.info("Imported rows=" + std::to_string(imported));
    logger.info(
        "Import rate=" +
        std::to_string(static_cast<std::uint64_t>(static_cast<double>(imported) / elapsed_sec)) +
        " rows/s"
    );

    if (!first_error.empty() && cfg.output.database.error_policy == core::ErrorPolicy::Stop) {
        throw std::runtime_error(first_error);
    }

    OutputStats stats;
    stats.execution_info = generate_result.info;
    stats.rows_generated = generated;
    stats.rows_written = imported;
    return stats;
}

}  // namespace data_generator::output
