// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file configuration.cpp

#include "core/configuration.h"

#include <set>
#include <unordered_set>

#include "cli/generator_catalog.h"
#include "generators/linkage_helper.h"

namespace data_generator::core {

namespace {

void add_issue(std::vector<ValidationIssue>& issues, std::string path, std::string message) {
    issues.emplace_back(ValidationIssue{std::move(path), std::move(message)});
}

bool parse_rows(const Json& root, ParseMode mode, int* rows, std::vector<ValidationIssue>& issues) {
    if (!root.contains("rows")) {
        if (mode == ParseMode::RequireOutputSettings) {
            add_issue(issues, "$.rows", "missing required integer");
            return false;
        }
        return true;
    }
    if (!root.at("rows").is_number_integer()) {
        add_issue(issues, "$.rows", "must be an integer");
        return false;
    }
    const int value = root.at("rows").get<int>();
    if (value <= 0) {
        add_issue(issues, "$.rows", "must be a positive integer");
        return false;
    }
    *rows = value;
    return true;
}

bool parse_format(const Json& root, ParseMode mode, OutputFormat* format, std::vector<ValidationIssue>& issues) {
    if (!root.contains("output_format")) {
        if (mode == ParseMode::RequireOutputSettings) {
            add_issue(issues, "$.output_format", "missing required string (csv|json|sql)");
            return false;
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
        "rows",
        "output_format",
        "null_value_string",
        "table_name",
        "include_create_table",
        "fields",
    };
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (!kKnownKeys.contains(it.key())) { add_issue(issues, "$." + it.key(), "unknown root key"); }
    }
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
            {"name", "generator", "config", "unique", "data_linkage", "null_value", "default_value"};
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
            if (p.required && !config.contains(p.name)) {
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
        spec.unique       = field.value("unique", false);
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

    validate_known_root_keys(root, *issues);
    parse_rows(root, mode, &cfg.rows, *issues);
    parse_format(root, mode, &cfg.format, *issues);
    parse_null_policy(root, &cfg.null_policy, *issues);

    if (root.contains("table_name")) {
        if (!root.at("table_name").is_string() || root.at("table_name").get<std::string>().empty()) {
            add_issue(*issues, "$.table_name", "must be a non-empty string");
        } else {
            cfg.table_name = root.at("table_name").get<std::string>();
        }
    }
    if (root.contains("include_create_table")) {
        if (!root.at("include_create_table").is_boolean()) {
            add_issue(*issues, "$.include_create_table", "must be a boolean");
        } else {
            cfg.include_create_table = root.at("include_create_table").get<bool>();
        }
    }

    validate_and_collect_fields(root, &cfg.fields, *issues);

    if (cfg.fields.empty()) { add_issue(*issues, "$.fields", "must contain at least one field"); }

    if (!issues->empty()) { return false; }

    *out = std::move(cfg);
    return true;
}

void apply_cli_overrides(
    GenerationConfig*                  cfg,
    const std::optional<int>&          rows,
    const std::optional<OutputFormat>& format,
    const std::optional<std::string>&  table_name,
    const std::optional<bool>&         include_create_table
) {
    if (!cfg) { return; }
    if (rows.has_value()) { cfg->rows = *rows; }
    if (format.has_value()) { cfg->format = *format; }
    if (table_name.has_value()) { cfg->table_name = *table_name; }
    if (include_create_table.has_value()) { cfg->include_create_table = *include_create_table; }
}

}  // namespace data_generator::core
