// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_init.cpp

#include "cli/command_init.h"

#include <cxxopts.hpp>
#include <chrono>
#include <ctime>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "cli/generator_catalog.h"
#include "core/workspace.h"
#include "database/db_metadata.h"
#include "database/db_url_parser.h"
#include "database/driver/idatabase_driver.h"

namespace data_generator::cli {

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

std::string default_init_output_path() {
    return "config_" + now_compact_timestamp() + ".json";
}

std::string to_lower_ascii(std::string text) {
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

bool is_ascii_alpha_numeric(const unsigned char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

std::string normalize_identifier_for_match(const std::string& raw) {
    std::string normalized;
    normalized.reserve(raw.size());
    for (const char raw_char : raw) {
        const unsigned char ch = static_cast<unsigned char>(raw_char);
        if (!is_ascii_alpha_numeric(ch)) { continue; }
        normalized.push_back(static_cast<char>(std::tolower(ch)));
    }
    return normalized;
}

std::vector<std::string> tokenize_identifier_for_match(const std::string& raw) {
    std::vector<std::string> tokens;
    std::string              current;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(raw[i]);
        if (!is_ascii_alpha_numeric(ch)) {
            if (!current.empty()) {
                tokens.push_back(to_lower_ascii(current));
                current.clear();
            }
            continue;
        }
        if (!current.empty() && std::isupper(ch) &&
            std::islower(static_cast<unsigned char>(current.back()))) {
            tokens.push_back(to_lower_ascii(current));
            current.clear();
        }
        current.push_back(static_cast<char>(ch));
    }
    if (!current.empty()) { tokens.push_back(to_lower_ascii(current)); }
    return tokens;
}

const std::unordered_map<std::string, std::string>& generator_name_map() {
    static const std::unordered_map<std::string, std::string> kMap = [] {
        std::unordered_map<std::string, std::string> map;
        for (const auto& meta : get_generator_catalog()) {
            map.emplace(normalize_identifier_for_match(meta.name), meta.name);
        }
        return map;
    }();
    return kMap;
}

const std::unordered_map<std::string, std::string>& generator_alias_map() {
    static const std::unordered_map<std::string, std::string> kAlias = {
        {"id", "sequence"},
        {"seq", "sequence"},
        {"sequenceid", "sequence"},
        {"guid", "uuid"},
        {"zipcode", "postcode"},
        {"zip", "postcode"},
        {"zipcod", "postcode"},
        {"postalcode", "postcode"},
        {"postcode", "postcode"},
        {"comp", "company_name"},
        {"company", "company_name"},
        {"compname", "company_name"},
        {"companyname", "company_name"},
        {"corpname", "company_name"},
        {"bizname", "company_name"},
        {"dept", "department"},
        {"industryname", "industry"},
        {"ip", "ip_address"},
        {"ipaddr", "ip_address"},
        {"mac", "mac_address"},
        {"hostname", "hostname"},
        {"host", "hostname"},
        {"website", "url"},
        {"weburl", "url"},
        {"filepath", "file_path"},
        {"filename", "file_name"},
        {"fileext", "file_extension"},
        {"filedir", "file_directory"},
        {"address1", "address_line1"},
        {"addr1", "address_line1"},
        {"address2", "address_line2"},
        {"addr2", "address_line2"},
        {"address", "full_address"},
        {"fullname", "full_name"},
        {"fname", "first_name"},
        {"lname", "last_name"},
        {"firstname", "first_name"},
        {"lastname", "last_name"},
        {"tel", "phone_number"},
        {"telephone", "phone_number"},
        {"mobile", "phone_number"},
        {"cellphone", "phone_number"},
        {"phonenumber", "phone_number"},
        {"mail", "email"},
        {"emailaddr", "email"},
        {"emailaddress", "email"},
        {"job", "job_title"},
        {"title", "title"},
        {"gender", "gender"},
        {"dob", "date"},
        {"birthdate", "date"},
        {"dt", "datetime"},
        {"timestamp", "datetime"},
        {"amount", "decimal"},
        {"price", "decimal"},
        {"qty", "integer"},
        {"count", "integer"},
        {"num", "integer"},
    };
    return kAlias;
}

bool is_name_infer_compatible(
    const std::string              generator_name,
    const database::ColumnTypeFamily family
) {
    if (family == database::ColumnTypeFamily::Integer) {
        return generator_name == "integer" || generator_name == "decimal" || generator_name == "sequence";
    }
    if (family == database::ColumnTypeFamily::Decimal) {
        return generator_name == "decimal" || generator_name == "integer" || generator_name == "sequence";
    }
    if (family == database::ColumnTypeFamily::Date) {
        return generator_name == "date" || generator_name == "datetime";
    }
    if (family == database::ColumnTypeFamily::Time) {
        return generator_name == "time" || generator_name == "datetime";
    }
    if (family == database::ColumnTypeFamily::DateTime) {
        return generator_name == "datetime" || generator_name == "date" || generator_name == "time";
    }
    if (family == database::ColumnTypeFamily::Enum) { return generator_name == "enum_item"; }
    if (family == database::ColumnTypeFamily::Boolean) {
        return generator_name == "enum_item" || generator_name == "integer";
    }
    if (family == database::ColumnTypeFamily::Binary) { return generator_name == "text"; }
    return true;
}

std::optional<std::string> infer_generator_name_from_column_name(const database::ColumnMetadata& column) {
    const std::string normalized_column = normalize_identifier_for_match(column.name);
    if (normalized_column.empty()) { return std::nullopt; }

    const auto& name_map = generator_name_map();
    if (const auto exact = name_map.find(normalized_column); exact != name_map.end()) {
        return exact->second;
    }

    const auto& alias_map = generator_alias_map();
    if (const auto alias = alias_map.find(normalized_column); alias != alias_map.end()) {
        return alias->second;
    }

    const auto family = database::classify_column_type(column);
    const auto tokens = tokenize_identifier_for_match(column.name);
    if (tokens.empty()) { return std::nullopt; }

    std::unordered_map<std::string, int> scores;
    for (const auto& token : tokens) {
        const std::string normalized_token = normalize_identifier_for_match(token);
        if (normalized_token.empty()) { continue; }
        if (const auto it = alias_map.find(normalized_token); it != alias_map.end()) {
            scores[it->second] += 8;
        }
        if (const auto it = name_map.find(normalized_token); it != name_map.end()) {
            scores[it->second] += 10;
        }
    }

    for (const auto& meta : get_generator_catalog()) {
        if (!is_name_infer_compatible(meta.name, family)) { continue; }

        const std::string normalized_generator = normalize_identifier_for_match(meta.name);
        if (normalized_generator.empty()) { continue; }

        if ((normalized_column.size() >= 4 && normalized_column.find(normalized_generator) != std::string::npos) ||
            (normalized_generator.size() >= 4 && normalized_generator.find(normalized_column) != std::string::npos)) {
            scores[meta.name] += 6;
        }

        const auto generator_tokens = tokenize_identifier_for_match(meta.name);
        for (const auto& column_token : tokens) {
            const std::string normalized_token = normalize_identifier_for_match(column_token);
            if (normalized_token.empty()) { continue; }
            for (const auto& generator_token : generator_tokens) {
                if (normalized_token == generator_token) {
                    scores[meta.name] += 4;
                } else if (normalized_token.size() >= 3 &&
                           generator_token.rfind(normalized_token, 0) == 0) {
                    scores[meta.name] += 2;
                } else if (generator_token.size() >= 3 &&
                           normalized_token.rfind(generator_token, 0) == 0) {
                    scores[meta.name] += 2;
                }
            }
        }
    }

    int         best_score = 0;
    std::string best_generator;
    for (const auto& [generator_name, score] : scores) {
        if (score > best_score) {
            best_score = score;
            best_generator = generator_name;
        }
    }
    if (best_score >= 8 && !best_generator.empty()) { return best_generator; }

    return std::nullopt;
}

std::string infer_generator_name(const database::ColumnMetadata& column) {
    if (const auto inferred = infer_generator_name_from_column_name(column); inferred.has_value()) {
        return *inferred;
    }

    const auto family = database::classify_column_type(column);
    switch (family) {
    case database::ColumnTypeFamily::Integer : return "integer";
    case database::ColumnTypeFamily::Decimal : return "decimal";
    case database::ColumnTypeFamily::Date    : return "date";
    case database::ColumnTypeFamily::Time    : return "time";
    case database::ColumnTypeFamily::DateTime: return "datetime";
    case database::ColumnTypeFamily::Enum    :
    case database::ColumnTypeFamily::Boolean : return "enum_item";
    case database::ColumnTypeFamily::String  :
    case database::ColumnTypeFamily::Binary  :
    case database::ColumnTypeFamily::Unknown : return "text";
    }
    return "text";
}

bool is_unique_column(const database::TableMetadata& metadata, const std::string& column_name) {
    for (const auto& index : metadata.indexes) {
        if (!index.unique) { continue; }
        if (index.columns.size() == 1 && index.columns.front() == column_name) { return true; }
    }
    return false;
}

OrderedJson make_null_value_defaults() {
    return OrderedJson{{"enabled", false}, {"percent", 0}};
}

OrderedJson make_default_value_defaults() {
    return OrderedJson{{"enabled", false}, {"percent", 0}, {"value", ""}};
}

void apply_supported_field_attributes(
    const database::TableMetadata&                   metadata,
    const database::ColumnMetadata&                  column,
    const GeneratorMetadata*                         meta,
    std::unordered_map<std::string, int>*            linkage_counter_by_generator,
    OrderedJson*                                     field
) {
    if (!meta || !field) { return; }

    if (meta->supports_unique) {
        const bool unique = column.auto_increment || is_unique_column(metadata, column.name);
        (*field)["unique"] = unique;
    }

    if (meta->supports_data_linkage && linkage_counter_by_generator) {
        const std::string module = meta->linkage_module.empty() ? meta->module : meta->linkage_module;
        const std::string group_module = module.empty() ? meta->name : module;
        const int group_id = ++(*linkage_counter_by_generator)[meta->name];
        (*field)["data_linkage"] = group_module + ":Group" + std::to_string(group_id);
    }

    (*field)["default_value"] = make_default_value_defaults();
    (*field)["null_value"] = make_null_value_defaults();
}

OrderedJson infer_field_from_column(
    const database::TableMetadata&                   metadata,
    const database::ColumnMetadata&                  column,
    std::unordered_map<std::string, int>*            linkage_counter_by_generator
) {
    OrderedJson field = OrderedJson::object();
    const std::string generator_name = infer_generator_name(column);

    const GeneratorMetadata* meta = find_generator_metadata(generator_name);

    field["name"] = column.name;
    field["generator"] = generator_name;
    field["config"] = meta ? build_ordered_config_template(*meta) : OrderedJson::object();

    if (generator_name == "integer") {
        if (column.unsigned_number) {
            field["config"]["start"] = 0;
        }
        if (column.numeric_precision.has_value()) {
            const int digits = std::min(9, *column.numeric_precision);
            int max_value = 1;
            for (int i = 0; i < digits; ++i) { max_value *= 10; }
            field["config"]["end"] = max_value - 1;
        }
    } else if (generator_name == "sequence") {
        if (column.numeric_precision.has_value()) {
            const int digits = std::min(18, *column.numeric_precision);
            std::uint64_t max_value = 1;
            for (int i = 0; i < digits; ++i) { max_value *= 10ULL; }
            field["config"]["end"] = max_value - 1ULL;
        }
        field["config"]["circle"] = false;
    } else if (generator_name == "decimal") {
        const int scale = column.numeric_scale.value_or(2);
        field["config"]["decimal_places"] = std::max(0, scale);
        if (column.unsigned_number) {
            field["config"]["start"] = 0.0;
        }
    } else if (generator_name == "text") {
        if (column.character_length.has_value()) {
            const int max_len = std::max(1, *column.character_length);
            field["config"]["number_of_chars_start"] = std::min(16, max_len);
            field["config"]["number_of_chars_end"] = max_len;
        }
    } else if (generator_name == "enum_item") {
        OrderedJson enums = OrderedJson::array();
        if (!column.enum_values.empty()) {
            for (const auto& enum_value : column.enum_values) { enums.push_back(enum_value); }
        } else {
            const database::ColumnTypeFamily family = database::classify_column_type(column);
            const bool one_char_limit = column.character_length.has_value() && *column.character_length <= 1;
            if (one_char_limit || family == database::ColumnTypeFamily::Boolean) {
                enums.push_back("1");
                enums.push_back("0");
            } else {
                enums.push_back("true");
                enums.push_back("false");
            }
        }
        field["config"]["enums"] = std::move(enums);
    }

    apply_supported_field_attributes(metadata, column, meta, linkage_counter_by_generator, &field);

    return field;
}

OrderedJson build_fields_from_template(const Json& root) {
    OrderedJson fields = OrderedJson::array();
    if (!root.contains("fields") || !root.at("fields").is_array()) { return fields; }

    std::unordered_map<std::string, int> linkage_counter_by_generator;

    for (const auto& field : root.at("fields")) {
        OrderedJson ordered_field = OrderedJson::object();
        if (field.contains("name")) { ordered_field["name"] = field.at("name"); }
        if (field.contains("generator")) { ordered_field["generator"] = field.at("generator"); }
        if (field.contains("config")) { ordered_field["config"] = to_ordered_json(field.at("config")); }
        const std::string generator_name = field.contains("generator") && field.at("generator").is_string()
                                               ? field.at("generator").get<std::string>()
                                               : "";
        const GeneratorMetadata* meta = generator_name.empty() ? nullptr : find_generator_metadata(generator_name);
        database::ColumnMetadata pseudo_column;
        pseudo_column.name = field.value("name", "");
        database::TableMetadata pseudo_metadata;
        apply_supported_field_attributes(
            pseudo_metadata,
            pseudo_column,
            meta,
            &linkage_counter_by_generator,
            &ordered_field
        );
        fields.push_back(std::move(ordered_field));
    }

    return fields;
}

}  // namespace

int CommandInit::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator init", "Generate JSON configuration template.");
    options.add_options()
        ("generator", "Generator name", cxxopts::value<std::string>())
        ("output", "Output file path", cxxopts::value<std::string>())
        ("rows", "Number of rows", cxxopts::value<int>())
        ("output-dest", "Output destination (file|database)", cxxopts::value<std::string>()->default_value("file"))
        ("format", "Output format (json|csv|sql), file mode only", cxxopts::value<std::string>())
        ("url", "Database URL or ODBC connection string (database mode)", cxxopts::value<std::string>())
        ("table", "Target table name (database mode / sql mode)", cxxopts::value<std::string>())
        ("workspace", "Workspace root path", cxxopts::value<std::string>())
        ("h,help", "Show help");

    cxxopts::ParseResult result;
    try {
        result = parse_options(options, args);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to parse arguments: " << ex.what() << "\n";
        std::cerr << options.help() << "\n";
        return exit_codes::kUsage;
    }

    if (result.count("help")) {
        std::cout << options.help() << "\n";
        return exit_codes::kOk;
    }

    int rows = 100;
    if (result.count("rows")) {
        rows = result["rows"].as<int>();
        if (rows <= 0) {
            std::cerr << "rows must be a positive integer.\n";
            return exit_codes::kUsage;
        }
    }

    std::string output_dest = result["output-dest"].as<std::string>();
    if (output_dest != "file" && output_dest != "database") {
        std::cerr << "--output-dest must be file or database\n";
        return exit_codes::kUsage;
    }

    std::string output_format = "csv";
    if (result.count("format")) {
        if (output_dest != "file") {
            std::cerr << "--format only works with --output-dest file\n";
            return exit_codes::kUsage;
        }
        output_format = result["format"].as<std::string>();
        if (output_format != "csv" && output_format != "json" && output_format != "sql") {
            std::cerr << "Unsupported format: " << output_format << "\n";
            return exit_codes::kUsage;
        }
    }

    const std::string workspace = result.count("workspace") ? result["workspace"].as<std::string>()
                                                             : core::default_workspace_root().string();
    if (workspace.empty()) {
        std::cerr << "workspace must not be empty\n";
        return exit_codes::kUsage;
    }

    const std::string default_url =
        "odbc:mysql:DRIVER={MySQL ODBC 8.0 Unicode Driver};SERVER=127.0.0.1;PORT=3306;DATABASE=example_db;UID=user;PWD=password;";
    const std::string default_table = "generated_data";

    const std::string url = result.count("url") ? result["url"].as<std::string>() : default_url;
    const std::string table = result.count("table") ? result["table"].as<std::string>() : default_table;

    OrderedJson output = OrderedJson::object();
    output["$schema"] = "./schema/data-generator.schema.json";
    output["workspace"] = workspace;
    output["rows"] = rows;
    output["output_dest"] = output_dest;
    output["null_value_string"] = "";
    output["insert_mode"] = "auto";
    output["batch_size"] = 1000;
    output["queue_size"] = 1024;
    output["db_threads"] = 2;
    output["transaction_mode"] = "per-batch";
    output["error_policy"] = "stop";
    output["rate_limit_rows_per_sec"] = 20000;

    OrderedJson fields = OrderedJson::array();

    if (output_dest == "database") {
        output["url"] = url;
        output["table"] = table;

        if (result.count("url") && result.count("table")) {
            database::DbUrl parsed_url;
            std::string     error;
            if (!database::parse_db_url(url, &parsed_url, &error)) {
                std::cerr << "Invalid --url: " << error << "\n";
                return exit_codes::kUsage;
            }

            auto driver = database::make_database_driver(parsed_url.type);
            if (!driver) {
                std::cerr << "Unsupported db type in URL\n";
                return exit_codes::kUsage;
            }
            if (!driver->connect(parsed_url, &error)) {
                std::cerr << "Failed to connect database: " << error << "\n";
                return exit_codes::kRuntimeFailure;
            }

            database::TableMetadata metadata;
            if (!driver->get_table_metadata(table, &metadata, &error)) {
                driver->disconnect();
                std::cerr << "Failed to read table metadata: " << error << "\n";
                return exit_codes::kRuntimeFailure;
            }

            std::unordered_map<std::string, int> linkage_counter_by_generator;
            for (const auto& column : metadata.columns) {
                fields.push_back(infer_field_from_column(metadata, column, &linkage_counter_by_generator));
            }
            driver->disconnect();
        }

        if (fields.empty()) {
            const Json root_template = build_project_template(rows, "csv");
            fields = build_fields_from_template(root_template);
        }
    } else {
        output["output_format"] = output_format;
        if (output_format == "sql") {
            output["table"] = table;
        }

        if (result.count("generator")) {
            const std::string        generator_name = result["generator"].as<std::string>();
            const GeneratorMetadata* meta           = find_generator_metadata(generator_name);
            if (!meta) {
                std::cerr << "Unknown generator: " << generator_name << "\n";
                return exit_codes::kUsage;
            }
            OrderedJson field = OrderedJson::object();
            field["name"] = "field_1";
            field["generator"] = meta->name;
            field["config"] = build_ordered_config_template(*meta);
            std::unordered_map<std::string, int> linkage_counter_by_generator;
            database::ColumnMetadata             pseudo_column;
            pseudo_column.name = "field_1";
            database::TableMetadata pseudo_metadata;
            apply_supported_field_attributes(
                pseudo_metadata,
                pseudo_column,
                meta,
                &linkage_counter_by_generator,
                &field
            );
            fields.push_back(std::move(field));
        } else {
            const Json root_template = build_project_template(rows, output_format);
            fields = build_fields_from_template(root_template);
        }
    }

    output["fields"] = std::move(fields);

    const std::string path = result.count("output") ? result["output"].as<std::string>() : default_init_output_path();

    const std::string payload = output.dump(2);
    std::ofstream     out(path, std::ios::trunc);
    if (!out) {
        std::cerr << "Failed to open output file: " << path << "\n";
        return exit_codes::kCliError;
    }
    out << payload << "\n";

    std::cout << "Config written to: " << path << "\n";
    return exit_codes::kOk;
}

}  // namespace data_generator::cli
