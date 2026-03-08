// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file configuration.cpp

#include "core/configuration.h"

#include <set>
#include <unordered_set>

#include "cli/generator_catalog.h"
#include "core/workspace.h"
#include "generators/linkage_helper.h"

namespace data_generator::core {

namespace {

void add_issue(std::vector<ValidationIssue>& issues, std::string path, std::string message, const bool warning = false) {
    issues.emplace_back(ValidationIssue{.warning = warning, .path = std::move(path), .message = std::move(message)});
}

bool parse_positive_int(
    const Json& root,
    const char* key,
    int* out,
    std::vector<ValidationIssue>& issues,
    const int min_value,
    const bool required,
    const bool warning_if_missing = false
) {
    if (!root.contains(key)) {
        if (required) {
            add_issue(issues, std::string("$.") + key, "missing required integer");
            return false;
        }
        if (warning_if_missing) { add_issue(issues, std::string("$.") + key, "not specified, default will be used", true); }
        return true;
    }
    if (!root.at(key).is_number_integer()) {
        add_issue(issues, std::string("$.") + key, "must be an integer");
        return false;
    }
    const int value = root.at(key).get<int>();
    if (value < min_value) {
        add_issue(
            issues,
            std::string("$.") + key,
            "must be >= " + std::to_string(min_value)
        );
        return false;
    }
    *out = value;
    return true;
}

bool parse_rows(const Json& root, const ParseMode mode, int* rows, std::vector<ValidationIssue>& issues) {
    const bool required = (mode == ParseMode::RequireOutputSettings);
    return parse_positive_int(root, "rows", rows, issues, 1, required, !required);
}

bool parse_format(const Json& root, const ParseMode mode, OutputFormat* format, std::vector<ValidationIssue>& issues) {
    const std::string output_dest_key = root.contains("output_dest") ? "output_dest" : "output_destination";
    const bool database_dest = root.contains(output_dest_key) && root.at(output_dest_key).is_string() &&
                               root.at(output_dest_key).get<std::string>() == "database";

    if (!root.contains("output_format")) {
        if (mode == ParseMode::RequireOutputSettings && !database_dest) {
            add_issue(issues, "$.output_format", "missing required string (csv|json|sql)");
            return false;
        }
        if (!database_dest) {
            add_issue(issues, "$.output_format", "not specified, default csv will be used", true);
        }
        return true;
    }
    if (!root.at("output_format").is_string()) {
        add_issue(issues, "$.output_format", "must be a string (csv|json|sql)");
        return false;
    }
    const auto parsed = parse_output_format(root.at("output_format").get<std::string>());
    if (!parsed.has_value()) {
        add_issue(issues, "$.output_format", "must be one of: csv, json, sql");
        return false;
    }
    *format = *parsed;
    return true;
}

void parse_null_policy(const Json& root, NullPolicy* policy, std::vector<ValidationIssue>& issues) {
    if (!root.contains("null_value_string")) { return; }
    policy->configured = true;
    const auto& value  = root.at("null_value_string");
    if (value.is_null()) {
        policy->null_if_empty = true;
        policy->null_literal.reset();
        return;
    }
    if (value.is_string()) {
        policy->null_literal = value.get<std::string>();
        return;
    }
    add_issue(issues, "$.null_value_string", "must be null or string");
}

void validate_known_root_keys(const Json& root, std::vector<ValidationIssue>& issues) {
    static const std::unordered_set<std::string> kKnownKeys = {
        "$schema",
        "workspace",
        "rows",
        "output_format",
        "null_value_string",
        "table",
        "table_name",
        "fields",
        "output_dest",
        "output_destination",
        "output_path",
        "url",
        "database_url",
        "odbc_connection",
        "insert_mode",
        "batch_size",
        "queue_size",
        "db_threads",
        "transaction_mode",
        "error_policy",
        "rate_limit_rows_per_sec",
    };
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!kKnownKeys.contains(it.key())) { add_issue(issues, "$." + it.key(), "unknown root key"); }
    }
}

void parse_workspace_setting(
    const Json&                   root,
    GenerationConfig*             cfg,
    std::vector<ValidationIssue>& issues
) {
    if (!root.contains("workspace")) { return; }
    if (!root.at("workspace").is_string() || root.at("workspace").get<std::string>().empty()) {
        add_issue(issues, "$.workspace", "must be a non-empty string");
        return;
    }
    cfg->workspace = root.at("workspace").get<std::string>();
}

bool parse_output_settings(
    const Json&                   root,
    const ParseMode               mode,
    GenerationConfig*             cfg,
    std::vector<ValidationIssue>& issues
) {
    const std::string output_dest_key = root.contains("output_dest") ? "output_dest" : "output_destination";
    if (root.contains(output_dest_key)) {
        if (!root.at(output_dest_key).is_string()) {
            add_issue(issues, "$." + output_dest_key, "must be string (file|database)");
        } else {
            const auto parsed = parse_output_destination(root.at(output_dest_key).get<std::string>());
            if (!parsed.has_value()) {
                add_issue(issues, "$." + output_dest_key, "must be one of: file, database");
            } else {
                cfg->output.destination = *parsed;
            }
        }
    }

    if (root.contains("output_path")) {
        if (!root.at("output_path").is_string()) {
            add_issue(issues, "$.output_path", "must be a string");
        } else {
            cfg->output.file.path = root.at("output_path").get<std::string>();
        }
    }

    std::string connection_key;
    if (root.contains("url")) {
        connection_key = "url";
    } else if (root.contains("database_url")) {
        connection_key = "database_url";
    } else if (root.contains("odbc_connection")) {
        connection_key = "odbc_connection";
    }
    if (!connection_key.empty()) {
        if (!root.at(connection_key).is_string() || root.at(connection_key).get<std::string>().empty()) {
            add_issue(issues, "$." + connection_key, "must be a non-empty database URL/ODBC string");
        } else {
            cfg->output.database.url = root.at(connection_key).get<std::string>();
        }
    }

    const std::string table_key =
        root.contains("table") ? "table" : (root.contains("database_table") ? "database_table" : "table_name");
    if (root.contains(table_key)) {
        if (!root.at(table_key).is_string() || root.at(table_key).get<std::string>().empty()) {
            add_issue(issues, "$." + table_key, "must be a non-empty string");
        } else {
            cfg->table_name = root.at(table_key).get<std::string>();
            cfg->output.database.table_name = cfg->table_name;
        }
    }

    if (root.contains("insert_mode")) {
        if (!root.at("insert_mode").is_string()) {
            add_issue(issues, "$.insert_mode", "must be a string (auto|insert|bulk|load)");
        } else {
            const auto parsed = parse_insert_mode(root.at("insert_mode").get<std::string>());
            if (!parsed.has_value()) {
                add_issue(issues, "$.insert_mode", "must be one of: auto, insert, bulk, load");
            } else {
                cfg->output.database.insert_mode = *parsed;
            }
        }
    }

    if (root.contains("transaction_mode")) {
        if (!root.at("transaction_mode").is_string()) {
            add_issue(issues, "$.transaction_mode", "must be a string (per-batch|per-run|none)");
        } else {
            const auto parsed = parse_transaction_mode(root.at("transaction_mode").get<std::string>());
            if (!parsed.has_value()) {
                add_issue(issues, "$.transaction_mode", "must be one of: per-batch, per-run, none");
            } else {
                cfg->output.database.transaction_mode = *parsed;
            }
        }
    }

    if (root.contains("error_policy")) {
        if (!root.at("error_policy").is_string()) {
            add_issue(issues, "$.error_policy", "must be a string (stop|continue|rollback-batch|rollback-all)");
        } else {
            const auto parsed = parse_error_policy(root.at("error_policy").get<std::string>());
            if (!parsed.has_value()) {
                add_issue(issues, "$.error_policy", "must be one of: stop, continue, rollback-batch, rollback-all");
            } else {
                cfg->output.database.error_policy = *parsed;
            }
        }
    }

    (void)parse_positive_int(root, "batch_size", &cfg->output.database.batch_size, issues, 1, false);
    (void)parse_positive_int(root, "queue_size", &cfg->output.database.queue_size, issues, 1, false);
    (void)parse_positive_int(root, "db_threads", &cfg->output.database.db_threads, issues, 1, false);
    (void)parse_positive_int(root, "rate_limit_rows_per_sec", &cfg->output.database.rate_limit_rows_per_sec, issues, 1, false);

    if (cfg->output.destination == OutputDestination::Database) {
        if (cfg->output.database.url.empty()) {
            add_issue(
                issues,
                "$.url",
                "database output requires URL/ODBC connection string (CLI or JSON)",
                mode == ParseMode::AllowMissingOutputSettings
            );
        }
    }

    if (cfg->output.destination == OutputDestination::Database && root.contains("output_format")) {
        add_issue(issues, "$.output_format", "ignored when output_dest=database", true);
    }

    return true;
}

void validate_and_collect_fields(
    const Json&                   root,
    std::vector<FieldSpec>*       fields,
    std::vector<ValidationIssue>& issues
) {
    if (!root.contains("fields")) {
        add_issue(issues, "$.fields", "missing required array");
        return;
    }
    if (!root.at("fields").is_array()) {
        add_issue(issues, "$.fields", "must be an array");
        return;
    }

    for (size_t i = 0; i < root.at("fields").size(); ++i) {
        const std::string path  = "$.fields[" + std::to_string(i) + "]";
        const auto&       field = root.at("fields").at(i);
        if (!field.is_object()) {
            add_issue(issues, path, "must be an object");
            continue;
        }

        if (!field.contains("name") || !field.at("name").is_string()) {
            add_issue(issues, path + ".name", "must be a string");
        }
        if (!field.contains("generator") || !field.at("generator").is_string()) {
            add_issue(issues, path + ".generator", "must be a string");
        }
        if (!field.contains("config") || !field.at("config").is_object()) {
            add_issue(issues, path + ".config", "must be an object");
        }

        if (!field.contains("name") ||
            !field.at("name").is_string() ||
            !field.contains("generator") ||
            !field.at("generator").is_string() ||
            !field.contains("config") ||
            !field.at("config").is_object()) {
            continue;
        }

        const std::string generator = field.at("generator").get<std::string>();
        const auto*       meta      = cli::find_generator_metadata(generator);
        if (!meta) {
            add_issue(issues, path + ".generator", "unknown generator: " + generator);
            continue;
        }

        static const std::unordered_set<std::string> kFieldKeys =
            {"name", "generator", "config", "unique", "nullable", "data_linkage", "null_value", "default_value"};
        for (auto it = field.begin(); it != field.end(); ++it) {
            if (!kFieldKeys.contains(it.key())) { add_issue(issues, path + "." + it.key(), "unknown field key"); }
        }

        if (field.contains("unique")) {
            if (!field.at("unique").is_boolean()) {
                add_issue(issues, path + ".unique", "must be a boolean");
            } else if (!meta->supports_unique) {
                add_issue(issues, path + ".unique", "generator does not support unique");
            }
        }

        if (field.contains("nullable")) {
            if (!field.at("nullable").is_boolean()) { add_issue(issues, path + ".nullable", "must be a boolean"); }
        }

        std::optional<std::string> linkage;
        if (field.contains("data_linkage")) {
            try {
                linkage = generator::parse_linkage_key(field);
            } catch (const std::exception& ex) { add_issue(issues, path + ".data_linkage", ex.what()); }
            if (!meta->supports_data_linkage) {
                add_issue(issues, path + ".data_linkage", "generator does not support data_linkage");
            }
        }

        std::set<std::string> allowed;
        for (const auto& p : meta->config_params) { allowed.insert(p.name); }

        const auto& config = field.at("config");
        for (auto it = config.begin(); it != config.end(); ++it) {
            if (!allowed.contains(it.key())) { add_issue(issues, path + ".config." + it.key(), "unknown config key"); }
        }

        for (const auto& p : meta->config_params) {
            const bool has_default = meta->config_template.contains(p.name);
            if (p.required && !config.contains(p.name) && !has_default) {
                add_issue(issues, path + ".config." + p.name, "missing required config key");
            }
        }

        if (field.contains("null_value")) {
            if (!field.at("null_value").is_object()) { add_issue(issues, path + ".null_value", "must be an object"); }
        }

        if (field.contains("default_value")) {
            if (!field.at("default_value").is_object()) {
                add_issue(issues, path + ".default_value", "must be an object");
            }
        }

        FieldSpec spec;
        spec.name         = field.at("name").get<std::string>();
        spec.generator    = generator;
        spec.raw          = field;
        spec.data_linkage = linkage;
        spec.unique       = field.contains("unique") && field.at("unique").is_boolean() && field.at("unique").get<bool>();
        fields->emplace_back(std::move(spec));
    }
}

}  // namespace

std::optional<OutputFormat> parse_output_format(const std::string& value) {
    if (value == "csv") { return OutputFormat::Csv; }
    if (value == "json") { return OutputFormat::Json; }
    if (value == "sql") { return OutputFormat::Sql; }
    return std::nullopt;
}

std::string output_format_to_string(const OutputFormat format) {
    switch (format) {
    case OutputFormat::Csv : return "csv";
    case OutputFormat::Json: return "json";
    case OutputFormat::Sql : return "sql";
    }
    return "csv";
}

std::optional<OutputDestination> parse_output_destination(const std::string& value) {
    if (value == "file") { return OutputDestination::File; }
    if (value == "database") { return OutputDestination::Database; }
    return std::nullopt;
}

std::string output_destination_to_string(const OutputDestination destination) {
    switch (destination) {
    case OutputDestination::File    : return "file";
    case OutputDestination::Database: return "database";
    }
    return "file";
}

std::optional<InsertMode> parse_insert_mode(const std::string& value) {
    if (value == "auto") { return InsertMode::Auto; }
    if (value == "insert") { return InsertMode::Insert; }
    if (value == "bulk") { return InsertMode::Bulk; }
    if (value == "load") { return InsertMode::Load; }
    return std::nullopt;
}

std::string insert_mode_to_string(const InsertMode mode) {
    switch (mode) {
    case InsertMode::Auto  : return "auto";
    case InsertMode::Insert: return "insert";
    case InsertMode::Bulk  : return "bulk";
    case InsertMode::Load  : return "load";
    }
    return "auto";
}

std::optional<TransactionMode> parse_transaction_mode(const std::string& value) {
    if (value == "per-batch") { return TransactionMode::PerBatch; }
    if (value == "per-run") { return TransactionMode::PerRun; }
    if (value == "none") { return TransactionMode::None; }
    return std::nullopt;
}

std::string transaction_mode_to_string(const TransactionMode mode) {
    switch (mode) {
    case TransactionMode::PerBatch: return "per-batch";
    case TransactionMode::PerRun  : return "per-run";
    case TransactionMode::None    : return "none";
    }
    return "per-batch";
}

std::optional<ErrorPolicy> parse_error_policy(const std::string& value) {
    if (value == "stop") { return ErrorPolicy::Stop; }
    if (value == "continue") { return ErrorPolicy::Continue; }
    if (value == "rollback-batch") { return ErrorPolicy::RollbackBatch; }
    if (value == "rollback-all") { return ErrorPolicy::RollbackAll; }
    return std::nullopt;
}

std::string error_policy_to_string(const ErrorPolicy policy) {
    switch (policy) {
    case ErrorPolicy::Stop         : return "stop";
    case ErrorPolicy::Continue     : return "continue";
    case ErrorPolicy::RollbackBatch: return "rollback-batch";
    case ErrorPolicy::RollbackAll  : return "rollback-all";
    }
    return "stop";
}

bool parse_generation_config(
    const Json&                   root,
    const ParseMode               mode,
    GenerationConfig*             out,
    std::vector<ValidationIssue>* issues
) {
    if (!out || !issues) { return false; }
    issues->clear();

    if (!root.is_object()) {
        add_issue(*issues, "$", "root must be a JSON object");
        return false;
    }

    GenerationConfig cfg;
    cfg.root = root;
    cfg.workspace = default_workspace_root().string();

    validate_known_root_keys(root, *issues);
    parse_workspace_setting(root, &cfg, *issues);
    (void)parse_rows(root, mode, &cfg.rows, *issues);
    (void)parse_format(root, mode, &cfg.format, *issues);
    parse_null_policy(root, &cfg.null_policy, *issues);

    (void)parse_output_settings(root, mode, &cfg, *issues);
    validate_and_collect_fields(root, &cfg.fields, *issues);

    if (cfg.fields.empty()) { add_issue(*issues, "$.fields", "must contain at least one field"); }

    bool has_error = false;
    for (const auto& issue : *issues) {
        if (!issue.warning) {
            has_error = true;
            break;
        }
    }
    if (has_error) { return false; }

    *out = std::move(cfg);
    return true;
}

void apply_cli_overrides(GenerationConfig* cfg, const CliOverrides& overrides) {
    if (!cfg) { return; }
    if (overrides.rows.has_value()) { cfg->rows = *overrides.rows; }
    if (overrides.format.has_value()) { cfg->format = *overrides.format; }
    if (overrides.table_name.has_value()) {
        cfg->table_name = *overrides.table_name;
        cfg->output.database.table_name = *overrides.table_name;
    }
    if (overrides.destination.has_value()) { cfg->output.destination = *overrides.destination; }
    if (overrides.output_path.has_value()) { cfg->output.file.path = *overrides.output_path; }
    if (overrides.database_url.has_value()) { cfg->output.database.url = *overrides.database_url; }
    if (overrides.insert_mode.has_value()) { cfg->output.database.insert_mode = *overrides.insert_mode; }
    if (overrides.batch_size.has_value()) { cfg->output.database.batch_size = *overrides.batch_size; }
    if (overrides.queue_size.has_value()) { cfg->output.database.queue_size = *overrides.queue_size; }
    if (overrides.db_threads.has_value()) { cfg->output.database.db_threads = *overrides.db_threads; }
    if (overrides.transaction_mode.has_value()) { cfg->output.database.transaction_mode = *overrides.transaction_mode; }
    if (overrides.error_policy.has_value()) { cfg->output.database.error_policy = *overrides.error_policy; }
    if (overrides.rate_limit_rows_per_sec.has_value()) {
        cfg->output.database.rate_limit_rows_per_sec = *overrides.rate_limit_rows_per_sec;
    }
    if (overrides.workspace.has_value()) { cfg->workspace = *overrides.workspace; }
}

}  // namespace data_generator::core
