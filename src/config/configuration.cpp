// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file configuration.cpp

#include "config/configuration.h"

#include <set>
#include <unordered_set>

#include "config/generator_catalog.h"
#include "generators/core/linkage_helper.h"
#include "utils/workspace.h"

namespace data_generator::config {

namespace {

constexpr auto kKeyRows = "rows";

constexpr auto kKeyOutput         = "output";
constexpr auto kKeyOutputType     = "type";
constexpr auto kKeyOutputFile     = "file";
constexpr auto kKeyOutputDatabase = "database";
constexpr auto kKeyOutputFormat   = "format";
constexpr auto kKeyOutputOptions  = "options";

constexpr auto kKeyFields = "fields";

constexpr auto kKeyFileOptionHeader      = "header";
constexpr auto kKeyFileOptionLineEnding  = "line_ending";
constexpr auto kKeyFileOptionArray       = "array";
constexpr auto kKeyFileOptionIncludeNull = "include_null";
constexpr auto kKeyFileOptionTable       = "table";
constexpr auto kKeyFileOptionCreateTable = "create_table";
constexpr auto kKeyFileOptionDelimiter   = "delimiter";
constexpr auto kKeyFileOptionQuote       = "quote";

constexpr auto kKeyDatabaseConnection          = "connection";
constexpr auto kKeyDatabaseTable               = "table";
constexpr auto kKeyDatabaseInsertMode          = "insert_mode";
constexpr auto kKeyDatabaseBatchSize           = "batch_size";
constexpr auto kKeyDatabaseQueueSize           = "queue_size";
constexpr auto kKeyDatabaseThreads             = "threads";
constexpr auto kKeyDatabaseTransactionMode     = "transaction_mode";
constexpr auto kKeyDatabaseErrorPolicy         = "error_policy";
constexpr auto kKeyDatabaseRateLimitRowsPerSec = "rate_limit_rows_per_sec";

const std::unordered_set<std::string> kKnownRootKeys = {
    "$schema",
    kKeyRows,
    kKeyOutput,
    kKeyFields,
};

const std::unordered_set<std::string> kKnownFieldKeys = {
    "name",
    "generator",
    "config",
    "unique",
    "data_linkage",
    "null_value",
    "default_value",
};

const std::unordered_set<std::string> kKnownOutputKeys = {
    kKeyOutputType,
    kKeyOutputFile,
    kKeyOutputDatabase,
};

const std::unordered_set<std::string> kKnownFileKeys = {
    kKeyOutputFormat,
    kKeyOutputOptions,
};

const std::unordered_set<std::string> kKnownDatabaseKeys = {
    kKeyDatabaseConnection,
    kKeyDatabaseTable,
    kKeyDatabaseInsertMode,
    kKeyDatabaseBatchSize,
    kKeyDatabaseQueueSize,
    kKeyDatabaseThreads,
    kKeyDatabaseTransactionMode,
    kKeyDatabaseErrorPolicy,
    kKeyDatabaseRateLimitRowsPerSec,
};

const std::unordered_set<std::string> kKnownCsvOptionKeys = {
    kKeyFileOptionHeader,
    kKeyFileOptionLineEnding,
};

const std::unordered_set<std::string> kKnownJsonOptionKeys = {
    kKeyFileOptionArray,
    kKeyFileOptionIncludeNull,
};

const std::unordered_set<std::string> kKnownSqlOptionKeys = {
    kKeyFileOptionTable,
    kKeyFileOptionCreateTable,
};

const std::unordered_set<std::string> kKnownCustomOptionKeys = {
    kKeyFileOptionDelimiter,
    kKeyFileOptionQuote,
    kKeyFileOptionHeader,
    kKeyFileOptionLineEnding,
};

void
    add_issue(std::vector<ValidationIssue>& issues, std::string path, std::string message, const bool warning = false) {
    issues.emplace_back(ValidationIssue{.warning = warning, .path = std::move(path), .message = std::move(message)});
}

bool parse_positive_int(
    const Json&                   root,
    const char*                   key,
    int*                          out,
    std::vector<ValidationIssue>& issues,
    const int                     min_value,
    const bool                    required,
    const bool                    warning_if_missing = false
) {
    if (!root.contains(key)) {
        if (required) {
            add_issue(issues, std::string("$.") + key, "missing required integer");
            return false;
        }
        if (warning_if_missing) {
            add_issue(issues, std::string("$.") + key, "not specified, default will be used", true);
        }
        return true;
    }
    if (!root.at(key).is_number_integer()) {
        add_issue(issues, std::string("$.") + key, "must be an integer");
        return false;
    }
    const int value = root.at(key).get<int>();
    if (value < min_value) {
        add_issue(issues, std::string("$.") + key, "must be >= " + std::to_string(min_value));
        return false;
    }
    *out = value;
    return true;
}

bool parse_rows(const Json& root, const ParseMode mode, int* rows, std::vector<ValidationIssue>& issues) {
    const bool required = mode == ParseMode::RequireOutputSettings;
    return parse_positive_int(root, kKeyRows, rows, issues, 1, required, !required);
}

bool parse_line_ending_value(
    const Json&                   value,
    LineEnding*                   ending,
    std::vector<ValidationIssue>& issues,
    const std::string&            path
) {
    if (!value.is_string()) {
        add_issue(issues, path, "must be a string (LF|CRLF)");
        return false;
    }
    const std::string text = value.get<std::string>();
    if (text == "LF") {
        *ending = LineEnding::LF;
        return true;
    }
    if (text == "CRLF") {
        *ending = LineEnding::CRLF;
        return true;
    }
    add_issue(issues, path, "must be one of: LF, CRLF");
    return false;
}

void validate_known_root_keys(const Json& root, std::vector<ValidationIssue>& issues) {
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!kKnownRootKeys.contains(it.key())) { add_issue(issues, "$." + it.key(), "unknown root key"); }
    }
}

void validate_known_object_keys(
    const Json&                            object,
    const std::unordered_set<std::string>& known,
    const std::string&                     path,
    std::vector<ValidationIssue>&          issues
) {
    if (!object.is_object()) { return; }
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (!known.contains(it.key())) { add_issue(issues, path + "." + it.key(), "unknown key"); }
    }
}

bool parse_optional_bool(
    const Json&                   object,
    const char*                   key,
    bool*                         out,
    std::vector<ValidationIssue>& issues,
    const std::string&            path
) {
    if (!object.contains(key)) { return true; }
    if (!object.at(key).is_boolean()) {
        add_issue(issues, path + "." + key, "must be a boolean");
        return false;
    }
    *out = object.at(key).get<bool>();
    return true;
}

bool parse_optional_string(
    const Json&                   object,
    const char*                   key,
    std::string*                  out,
    std::vector<ValidationIssue>& issues,
    const std::string&            path,
    const bool                    require_non_empty
) {
    if (!object.contains(key)) { return true; }
    if (!object.at(key).is_string()) {
        add_issue(issues, path + "." + key, "must be a string");
        return false;
    }
    const std::string value = object.at(key).get<std::string>();
    if (require_non_empty && value.empty()) {
        add_issue(issues, path + "." + key, "must be a non-empty string");
        return false;
    }
    *out = value;
    return true;
}

bool parse_file_options(
    const Json&                   options,
    const OutputFormat            format,
    FileOutputConfig*             file_cfg,
    std::vector<ValidationIssue>& issues,
    const std::string&            path
) {
    if (!options.is_object()) {
        add_issue(issues, path, "must be an object");
        return false;
    }

    switch (format) {
    case OutputFormat::Csv: {
        validate_known_object_keys(options, kKnownCsvOptionKeys, path, issues);
        (void)parse_optional_bool(options, kKeyFileOptionHeader, &file_cfg->csv.header, issues, path);
        if (options.contains(kKeyFileOptionLineEnding)) {
            (void)parse_line_ending_value(
                options.at(kKeyFileOptionLineEnding),
                &file_cfg->csv.line_ending,
                issues,
                path + "." + kKeyFileOptionLineEnding
            );
        }
        return true;
    }
    case OutputFormat::TabDelimited: {
        validate_known_object_keys(options, kKnownCsvOptionKeys, path, issues);
        (void)parse_optional_bool(options, kKeyFileOptionHeader, &file_cfg->tab_delimited.header, issues, path);
        if (options.contains(kKeyFileOptionLineEnding)) {
            (void)parse_line_ending_value(
                options.at(kKeyFileOptionLineEnding),
                &file_cfg->tab_delimited.line_ending,
                issues,
                path + "." + kKeyFileOptionLineEnding
            );
        }
        return true;
    }
    case OutputFormat::JsonFormat: {
        validate_known_object_keys(options, kKnownJsonOptionKeys, path, issues);
        (void)parse_optional_bool(options, kKeyFileOptionArray, &file_cfg->json.array, issues, path);
        (void)parse_optional_bool(options, kKeyFileOptionIncludeNull, &file_cfg->json.include_null, issues, path);
        return true;
    }
    case OutputFormat::Sql: {
        validate_known_object_keys(options, kKnownSqlOptionKeys, path, issues);
        (void)parse_optional_string(options, kKeyFileOptionTable, &file_cfg->sql.table, issues, path, true);
        (void)parse_optional_bool(options, kKeyFileOptionCreateTable, &file_cfg->sql.create_table, issues, path);
        return true;
    }
    case OutputFormat::Custom: {
        validate_known_object_keys(options, kKnownCustomOptionKeys, path, issues);
        (void)parse_optional_string(options, kKeyFileOptionDelimiter, &file_cfg->custom.delimiter, issues, path, true);
        (void)parse_optional_string(options, kKeyFileOptionQuote, &file_cfg->custom.quote, issues, path, false);
        (void)parse_optional_bool(options, kKeyFileOptionHeader, &file_cfg->custom.header, issues, path);
        if (options.contains(kKeyFileOptionLineEnding)) {
            (void)parse_line_ending_value(
                options.at(kKeyFileOptionLineEnding),
                &file_cfg->custom.line_ending,
                issues,
                path + "." + kKeyFileOptionLineEnding
            );
        }
        return true;
    }
    }
    return true;
}

bool parse_file_output(
    const Json&                   output,
    const ParseMode               mode,
    GenerationConfig*             cfg,
    std::vector<ValidationIssue>& issues
) {
    const std::string base_path = "$.output.file";
    if (!output.contains(kKeyOutputFile)) {
        add_issue(
            issues,
            base_path,
            "missing required object for file output",
            mode == ParseMode::AllowMissingOutputSettings
        );
        return mode == ParseMode::AllowMissingOutputSettings;
    }
    if (!output.at(kKeyOutputFile).is_object()) {
        add_issue(issues, base_path, "must be an object");
        return false;
    }
    const Json& file = output.at(kKeyOutputFile);
    validate_known_object_keys(file, kKnownFileKeys, base_path, issues);

    if (file.contains(kKeyOutputFormat)) {
        if (!file.at(kKeyOutputFormat).is_string()) {
            add_issue(issues, base_path + ".format", "must be a string");
        } else {
            const auto parsed = parse_output_format(file.at(kKeyOutputFormat).get<std::string>());
            if (!parsed.has_value()) {
                add_issue(issues, base_path + ".format", "must be one of: csv, json, sql, Tab-Delimited, Custom");
            } else {
                cfg->output.file.format = *parsed;
            }
        }
    } else if (mode == ParseMode::RequireOutputSettings) {
        add_issue(issues, base_path + ".format", "missing required format");
    }

    if (file.contains(kKeyOutputOptions)) {
        (void)parse_file_options(
            file.at(kKeyOutputOptions),
            cfg->output.file.format,
            &cfg->output.file,
            issues,
            base_path + ".options"
        );
    }

    return true;
}

bool parse_database_output(
    const Json&                   output,
    const ParseMode               mode,
    GenerationConfig*             cfg,
    std::vector<ValidationIssue>& issues
) {
    const std::string base_path = "$.output.database";
    if (!output.contains(kKeyOutputDatabase)) {
        add_issue(
            issues,
            base_path,
            "missing required object for database output",
            mode == ParseMode::AllowMissingOutputSettings
        );
        return mode == ParseMode::AllowMissingOutputSettings;
    }
    if (!output.at(kKeyOutputDatabase).is_object()) {
        add_issue(issues, base_path, "must be an object");
        return false;
    }
    const Json& database = output.at(kKeyOutputDatabase);
    validate_known_object_keys(database, kKnownDatabaseKeys, base_path, issues);

    (
        void
    )parse_optional_string(database, kKeyDatabaseConnection, &cfg->output.database.connection, issues, base_path, true);
    (void)parse_optional_string(database, kKeyDatabaseTable, &cfg->output.database.table, issues, base_path, true);

    if (database.contains(kKeyDatabaseInsertMode)) {
        if (!database.at(kKeyDatabaseInsertMode).is_string()) {
            add_issue(issues, base_path + ".insert_mode", "must be a string (auto|insert|bulk|load)");
        } else {
            const auto parsed = parse_insert_mode(database.at(kKeyDatabaseInsertMode).get<std::string>());
            if (!parsed.has_value()) {
                add_issue(issues, base_path + ".insert_mode", "must be one of: auto, insert, bulk, load");
            } else {
                cfg->output.database.insert_mode = *parsed;
            }
        }
    }

    if (database.contains(kKeyDatabaseTransactionMode)) {
        if (!database.at(kKeyDatabaseTransactionMode).is_string()) {
            add_issue(issues, base_path + ".transaction_mode", "must be a string (per-batch|per-run|none)");
        } else {
            const auto parsed = parse_transaction_mode(database.at(kKeyDatabaseTransactionMode).get<std::string>());
            if (!parsed.has_value()) {
                add_issue(issues, base_path + ".transaction_mode", "must be one of: per-batch, per-run, none");
            } else {
                cfg->output.database.transaction_mode = *parsed;
            }
        }
    }

    if (database.contains(kKeyDatabaseErrorPolicy)) {
        if (!database.at(kKeyDatabaseErrorPolicy).is_string()) {
            add_issue(
                issues,
                base_path + ".error_policy",
                "must be a string (stop|continue|rollback-batch|rollback-all)"
            );
        } else {
            const auto parsed = parse_error_policy(database.at(kKeyDatabaseErrorPolicy).get<std::string>());
            if (!parsed.has_value()) {
                add_issue(
                    issues,
                    base_path + ".error_policy",
                    "must be one of: stop, continue, rollback-batch, rollback-all"
                );
            } else {
                cfg->output.database.error_policy = *parsed;
            }
        }
    }

    (void)parse_positive_int(database, kKeyDatabaseBatchSize, &cfg->output.database.batch_size, issues, 1, false);
    (void)parse_positive_int(database, kKeyDatabaseQueueSize, &cfg->output.database.queue_size, issues, 1, false);
    (void)parse_positive_int(database, kKeyDatabaseThreads, &cfg->output.database.db_threads, issues, 1, false);
    (void)parse_positive_int(
        database,
        kKeyDatabaseRateLimitRowsPerSec,
        &cfg->output.database.rate_limit_rows_per_sec,
        issues,
        1,
        false
    );

    if (cfg->output.database.connection.empty()) {
        add_issue(
            issues,
            base_path + ".connection",
            "database output requires connection in odbc://... or sqlite://... format",
            mode == ParseMode::AllowMissingOutputSettings
        );
    }
    if (cfg->output.database.table.empty()) {
        add_issue(
            issues,
            base_path + ".table",
            "database output requires table name",
            mode == ParseMode::AllowMissingOutputSettings
        );
    }

    return true;
}

bool parse_output_settings(
    const Json&                   root,
    const ParseMode               mode,
    GenerationConfig*             cfg,
    std::vector<ValidationIssue>& issues
) {
    if (!root.contains(kKeyOutput)) {
        add_issue(
            issues,
            "$.output",
            "missing required output configuration",
            mode == ParseMode::AllowMissingOutputSettings
        );
        return mode == ParseMode::AllowMissingOutputSettings;
    }
    if (!root.at(kKeyOutput).is_object()) {
        add_issue(issues, "$.output", "must be an object");
        return false;
    }

    const Json& output = root.at(kKeyOutput);
    validate_known_object_keys(output, kKnownOutputKeys, "$.output", issues);

    if (output.contains(kKeyOutputType)) {
        if (!output.at(kKeyOutputType).is_string()) {
            add_issue(issues, "$.output.type", "must be string (file|database)");
        } else {
            const auto parsed = parse_output_type(output.at(kKeyOutputType).get<std::string>());
            if (!parsed.has_value()) {
                add_issue(issues, "$.output.type", "must be one of: file, database");
            } else {
                cfg->output.type = *parsed;
            }
        }
    } else if (mode == ParseMode::RequireOutputSettings) {
        add_issue(issues, "$.output.type", "missing required output type");
    }

    if (cfg->output.type == OutputType::Database) { return parse_database_output(output, mode, cfg, issues); }

    return parse_file_output(output, mode, cfg, issues);
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

    for (std::string::size_type i = 0; i < root.at("fields").size(); ++i) {
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
        const auto*       meta      = find_generator_metadata(generator);
        if (!meta) {
            add_issue(issues, path + ".generator", "unknown generator: " + generator);
            continue;
        }

        for (auto it = field.begin(); it != field.end(); ++it) {
            if (!kKnownFieldKeys.contains(it.key())) { add_issue(issues, path + "." + it.key(), "unknown field key"); }
        }

        if (field.contains("unique")) {
            if (!field.at("unique").is_boolean()) {
                add_issue(issues, path + ".unique", "must be a boolean");
            } else if (!meta->supports_unique) {
                add_issue(issues, path + ".unique", "generator does not support unique");
            }
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
        spec.unique = field.contains("unique") && field.at("unique").is_boolean() && field.at("unique").get<bool>();
        fields->emplace_back(std::move(spec));
    }
}

}  // namespace

std::optional<OutputFormat> parse_output_format(const std::string& value) {
    if (value == "csv") { return OutputFormat::Csv; }
    if (value == "json") { return OutputFormat::JsonFormat; }
    if (value == "sql") { return OutputFormat::Sql; }
    if (value == "Tab-Delimited") { return OutputFormat::TabDelimited; }
    if (value == "Custom") { return OutputFormat::Custom; }
    return std::nullopt;
}

std::string output_format_to_string(const OutputFormat format) {
    switch (format) {
    case OutputFormat::Csv         : return "csv";
    case OutputFormat::JsonFormat  : return "json";
    case OutputFormat::Sql         : return "sql";
    case OutputFormat::TabDelimited: return "Tab-Delimited";
    case OutputFormat::Custom      : return "Custom";
    default                        : return "csv";
    }
}

std::optional<OutputType> parse_output_type(const std::string& value) {
    if (value == "file") { return OutputType::File; }
    if (value == "database") { return OutputType::Database; }
    return std::nullopt;
}

std::string output_type_to_string(const OutputType type) {
    switch (type) {
    case OutputType::File    : return "file";
    case OutputType::Database: return "database";
    default                  : return "file";
    }
}

std::optional<LineEnding> parse_line_ending(const std::string& value) {
    if (value == "LF") { return LineEnding::LF; }
    if (value == "CRLF") { return LineEnding::CRLF; }
    return std::nullopt;
}

std::string line_ending_to_string(const LineEnding ending) {
    switch (ending) {
    case LineEnding::LF  : return "LF";
    case LineEnding::CRLF: return "CRLF";
    default              : return "LF";
    }
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
    default                : return "auto";
    }
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
    default                       : return "per-batch";
    }
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
    default                        : return "stop";
    }
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
    cfg.root      = root;
    cfg.workspace = utils::default_workspace_root().string();

    validate_known_root_keys(root, *issues);
    (void)parse_rows(root, mode, &cfg.rows, *issues);

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
    if (overrides.format.has_value()) { cfg->output.file.format = *overrides.format; }
    if (overrides.table_name.has_value()) {
        cfg->output.database.table = *overrides.table_name;
        cfg->output.file.sql.table = *overrides.table_name;
    }
    if (overrides.type.has_value()) { cfg->output.type = *overrides.type; }
    if (overrides.output_path.has_value()) { cfg->output.file.path = *overrides.output_path; }
    if (overrides.database_connection.has_value()) { cfg->output.database.connection = *overrides.database_connection; }
}

}  // namespace data_generator::config
