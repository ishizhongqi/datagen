// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file command_init.cpp

#include "cli/command_init.h"

#include <algorithm>
#include <cctype>
#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "cli/cli_shared.h"
#include "cli/exit_codes.h"
#include "config/generator_catalog.h"
#include "output/database/db_metadata.h"
#include "output/database/db_url_parser.h"
#include "output/database/drivers/idatabase_driver.h"

namespace data_generator::cli {

namespace {

std::string to_lower_ascii(std::string text) {
    for (char& ch : text) { ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); }
    return text;
}

bool is_ascii_alpha_numeric(const unsigned char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

std::string normalize_identifier_for_match(const std::string& raw) {
    std::string normalized;
    normalized.reserve(raw.size());
    for (const char raw_char : raw) {
        const auto ch = static_cast<unsigned char>(raw_char);
        if (!is_ascii_alpha_numeric(ch)) { continue; }
        normalized.push_back(static_cast<char>(std::tolower(ch)));
    }
    return normalized;
}

std::vector<std::string> tokenize_identifier_for_match(const std::string& raw) {
    std::vector<std::string> tokens;
    std::string              current;
    for (const char i : raw) {
        const auto ch = static_cast<unsigned char>(i);
        if (!is_ascii_alpha_numeric(ch)) {
            if (!current.empty()) {
                tokens.push_back(to_lower_ascii(current));
                current.clear();
            }
            continue;
        }
        if (!current.empty() && std::isupper(ch) && std::islower(static_cast<unsigned char>(current.back()))) {
            tokens.push_back(to_lower_ascii(current));
            current.clear();
        }
        current.push_back(static_cast<char>(ch));
    }
    if (!current.empty()) { tokens.push_back(to_lower_ascii(current)); }
    return tokens;
}

const std::unordered_map<std::string, std::string> kGeneratorAliasMap = {
    {"id", "sequence"},
    {"seq", "sequence"},
    {"sequenceid", "sequence"},
    {"guid", "uuid"},
    {"zipcode", "postcode"},
    {"zip", "postcode"},
    {"zipcod", "postcode"},
    {"postalcode", "postcode"},
    {"postal", "postcode"},
    {"postcode", "postcode"},
    {"comp", "company_name"},
    {"company", "company_name"},
    {"compname", "company_name"},
    {"companyname", "company_name"},
    {"corpname", "company_name"},
    {"bizname", "company_name"},
    {"org", "company_name"},
    {"organization", "company_name"},
    {"organisation", "company_name"},
    {"vendor", "company_name"},
    {"supplier", "company_name"},
    {"employer", "company_name"},
    {"dept", "department"},
    {"team", "department"},
    {"division", "department"},
    {"industryname", "industry"},
    {"sector", "industry"},
    {"ip", "ip_address"},
    {"ipaddr", "ip_address"},
    {"ipv4", "ip_address"},
    {"ipv6", "ip_address"},
    {"mac", "mac_address"},
    {"macaddr", "mac_address"},
    {"hostname", "hostname"},
    {"host", "hostname"},
    {"url", "url"},
    {"uri", "url"},
    {"link", "url"},
    {"website", "url"},
    {"homepage", "url"},
    {"webpage", "url"},
    {"weburl", "url"},
    {"filepath", "file_path"},
    {"fullpath", "file_path"},
    {"directory", "file_directory"},
    {"dir", "file_directory"},
    {"folder", "file_directory"},
    {"filename", "file_name"},
    {"basename", "file_name"},
    {"fileext", "file_extension"},
    {"extension", "file_extension"},
    {"ext", "file_extension"},
    {"suffix", "file_extension"},
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
    {"phone", "phone_number"},
    {"mail", "email"},
    {"emailaddr", "email"},
    {"emailaddress", "email"},
    {"givenname", "first_name"},
    {"forename", "first_name"},
    {"surname", "last_name"},
    {"familyname", "last_name"},
    {"job", "job_title"},
    {"jobtitle", "job_title"},
    {"title", "title"},
    {"gender", "gender"},
    {"dob", "date"},
    {"birthdate", "date"},
    {"dt", "datetime"},
    {"timestamp", "datetime"},
    {"createdat", "datetime"},
    {"updatedat", "datetime"},
    {"modifiedat", "datetime"},
    {"deletedat", "datetime"},
    {"createdon", "datetime"},
    {"updatedon", "datetime"},
    {"modifiedon", "datetime"},
    {"amount", "decimal"},
    {"price", "decimal"},
    {"qty", "integer"},
    {"count", "integer"},
    {"num", "integer"},
    {"province", "region"},
    {"cardno", "card_number"},
    {"ccnumber", "card_number"},
    {"description", "text"},
    {"comment", "text"},
    {"note", "text"},
    {"memo", "text"},
    {"content", "text"},
    {"message", "text"},
    {"body", "text"},
    {"details", "text"},
    {"remark", "text"},
    {"summary", "text"},
};

const std::unordered_map<std::string, std::string> kGeneratorKeywordMap = {
    {"comp", "company_name"},
    {"company", "company_name"},
    {"vendor", "company_name"},
    {"supplier", "company_name"},
    {"org", "company_name"},
    {"team", "department"},
    {"dept", "department"},
    {"division", "department"},
    {"sector", "industry"},
    {"ip", "ip_address"},
    {"ipaddr", "ip_address"},
    {"ipv4", "ip_address"},
    {"ipv6", "ip_address"},
    {"mac", "mac_address"},
    {"macaddr", "mac_address"},
    {"url", "url"},
    {"uri", "url"},
    {"link", "url"},
    {"website", "url"},
    {"homepage", "url"},
    {"webpage", "url"},
    {"directory", "file_directory"},
    {"folder", "file_directory"},
    {"dir", "file_directory"},
    {"extension", "file_extension"},
    {"ext", "file_extension"},
    {"suffix", "file_extension"},
    {"zip", "postcode"},
    {"postal", "postcode"},
    {"postcode", "postcode"},
    {"phone", "phone_number"},
    {"tel", "phone_number"},
    {"telephone", "phone_number"},
    {"mobile", "phone_number"},
    {"cellphone", "phone_number"},
    {"mail", "email"},
    {"surname", "last_name"},
    {"familyname", "last_name"},
    {"givenname", "first_name"},
    {"forename", "first_name"},
    {"job", "job_title"},
    {"jobtitle", "job_title"},
    {"province", "region"},
    {"description", "text"},
    {"comment", "text"},
    {"note", "text"},
    {"memo", "text"},
    {"content", "text"},
    {"message", "text"},
    {"body", "text"},
    {"details", "text"},
    {"remark", "text"},
    {"summary", "text"},
};

std::optional<std::string> find_generator_name_by_normalized_identifier(const std::string& normalized_identifier) {
    for (const auto& meta : config::get_generator_catalog()) {
        if (normalize_identifier_for_match(meta.name) == normalized_identifier) { return meta.name; }
    }
    return std::nullopt;
}

const std::unordered_map<std::string, std::string>& generator_alias_map() {
    return kGeneratorAliasMap;
}

const std::unordered_map<std::string, std::string>& generator_keyword_map() {
    return kGeneratorKeywordMap;
}

bool is_name_infer_compatible(const std::string& generator_name, const database::ColumnTypeFamily family) {
    if (family == database::ColumnTypeFamily::Integer) {
        return generator_name == "integer" || generator_name == "decimal" || generator_name == "sequence";
    }
    if (family == database::ColumnTypeFamily::Decimal) {
        return generator_name == "decimal" || generator_name == "integer" || generator_name == "sequence";
    }
    if (family == database::ColumnTypeFamily::Date) { return generator_name == "date" || generator_name == "datetime"; }
    if (family == database::ColumnTypeFamily::Time) { return generator_name == "time" || generator_name == "datetime"; }
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

    if (const auto exact = find_generator_name_by_normalized_identifier(normalized_column); exact.has_value()) {
        return *exact;
    }

    const auto& alias_map = generator_alias_map();
    if (const auto alias = alias_map.find(normalized_column); alias != alias_map.end()) { return alias->second; }

    const auto family = database::classify_column_type(column);
    const auto tokens = tokenize_identifier_for_match(column.name);
    if (tokens.empty()) { return std::nullopt; }

    const auto& keyword_map = generator_keyword_map();
    std::unordered_map<std::string, int> scores;
    for (const auto& token : tokens) {
        const std::string normalized_token = normalize_identifier_for_match(token);
        if (normalized_token.empty()) { continue; }
        if (const auto it = keyword_map.find(normalized_token); it != keyword_map.end()) { scores[it->second] += 8; }
        if (const auto generator_name = find_generator_name_by_normalized_identifier(normalized_token);
            generator_name.has_value()) {
            scores[*generator_name] += 10;
        }

        if (normalized_token.size() >= 4) {
            for (const auto& [keyword, generator_name] : keyword_map) {
                if (keyword.size() >= 3 && normalized_token.find(keyword) != std::string::npos) {
                    scores[generator_name] += 3;
                }
            }
        }
    }

    for (const auto& meta : config::get_generator_catalog()) {
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
                } else if (normalized_token.size() >= 3 && generator_token.rfind(normalized_token, 0) == 0) {
                    scores[meta.name] += 2;
                }
            }
        }
    }

    int         best_score = 0;
    std::string best_generator;
    for (const auto& [generator_name, score] : scores) {
        if (score > best_score) {
            best_score     = score;
            best_generator = generator_name;
        }
    }
    if (best_score >= 8 && !best_generator.empty()) { return best_generator; }

    return std::nullopt;
}

std::string infer_generator_name(const database::ColumnMetadata& column) {
    if (const auto inferred = infer_generator_name_from_column_name(column); inferred.has_value()) { return *inferred; }

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
    default                                  : return "text";
    }
}

bool is_unique_column(const database::TableMetadata& metadata, const std::string& column_name) {
    return std::ranges::any_of(metadata.indexes, [&](const database::IndexMetadata& index) {
        return index.unique && index.columns.size() == 1 && index.columns.front() == column_name;
    });
}

OrderedJson make_null_value_defaults() {
    return OrderedJson{{"enabled", false}, {"percent", 0}};
}

OrderedJson make_default_value_defaults() {
    return OrderedJson{{"enabled", false}, {"percent", 0}, {"value", ""}};
}

void apply_supported_field_attributes(
    const database::TableMetadata&        metadata,
    const database::ColumnMetadata&       column,
    const config::GeneratorMetadata&      meta,
    std::unordered_map<std::string, int>& linkage_counter_by_generator,
    OrderedJson&                          field
) {
    if (meta.supports_unique) {
        const bool unique = column.auto_increment || is_unique_column(metadata, column.name);
        field["unique"]   = unique;
    }

    if (meta.supports_data_linkage) {
        const std::string module       = meta.linkage_module.empty() ? meta.module : meta.linkage_module;
        const std::string group_module = module.empty() ? meta.name : module;
        const int         group_id     = ++linkage_counter_by_generator[meta.name];
        field["data_linkage"]          = group_module + ":Group" + std::to_string(group_id);
    }

    field["default_value"] = make_default_value_defaults();
    field["null_value"]    = make_null_value_defaults();
}

OrderedJson infer_field_from_column(
    const database::TableMetadata&        metadata,
    const database::ColumnMetadata&       column,
    std::unordered_map<std::string, int>* linkage_counter_by_generator
) {
    OrderedJson       field          = OrderedJson::object();
    const std::string generator_name = infer_generator_name(column);

    const config::GeneratorMetadata* meta = config::find_generator_metadata(generator_name);

    field["name"]      = column.name;
    field["generator"] = generator_name;
    field["config"]    = meta ? build_ordered_config_template(*meta) : OrderedJson::object();

    if (generator_name == "integer") {
        if (column.unsigned_number) { field["config"]["start"] = 0; }
        if (column.numeric_precision.has_value()) {
            const int digits    = std::min(9, *column.numeric_precision);
            int       max_value = 1;
            for (int i = 0; i < digits; ++i) { max_value *= 10; }
            field["config"]["end"] = max_value - 1;
        }
    } else if (generator_name == "sequence") {
        if (column.numeric_precision.has_value()) {
            const int     digits    = std::min(18, *column.numeric_precision);
            std::uint64_t max_value = 1;
            for (int i = 0; i < digits; ++i) { max_value *= 10ULL; }
            field["config"]["end"] = max_value - 1ULL;
        }
        field["config"]["circle"] = false;
    } else if (generator_name == "decimal") {
        const int scale                   = column.numeric_scale.value_or(2);
        field["config"]["decimal_places"] = std::max(0, scale);
        if (column.unsigned_number) { field["config"]["start"] = 0.0; }
    } else if (generator_name == "text") {
        if (column.character_length.has_value()) {
            const int max_len                        = std::max(1, *column.character_length);
            field["config"]["number_of_chars_start"] = std::min(16, max_len);
            field["config"]["number_of_chars_end"]   = max_len;
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

    if (meta && linkage_counter_by_generator) {
        apply_supported_field_attributes(metadata, column, *meta, *linkage_counter_by_generator, field);
    }

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
        const config::GeneratorMetadata* meta =
            generator_name.empty() ? nullptr : config::find_generator_metadata(generator_name);
        database::ColumnMetadata pseudo_column;
        pseudo_column.name = field.value("name", "");
        database::TableMetadata pseudo_metadata;
        if (meta) {
            apply_supported_field_attributes(
                pseudo_metadata,
                pseudo_column,
                *meta,
                linkage_counter_by_generator,
                ordered_field
            );
        }
        fields.push_back(std::move(ordered_field));
    }

    return fields;
}

}  // namespace

int CommandInit::run(const std::vector<std::string>& args) {
    cxxopts::Options options("data-generator init", "Generate JSON configuration template.");
    options.add_options()
        ("config", "Output JSON file path", cxxopts::value<std::string>())
        ("template", "Template type (file|database)", cxxopts::value<std::string>()->default_value("file"))
        ("format", "File format (csv|json|sql|Tab-Delimited|Custom)", cxxopts::value<std::string>())
        ("from-database", "Database connection in odbc://... or sqlite://... format to infer fields", cxxopts::value<std::string>())
        ("table", "Target table name", cxxopts::value<std::string>())
        ("h,help", "Show help");
    options.parse_positional({"config"});

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

    if (!result.count("config")) {
        std::cerr << "Missing required <json>\n";
        std::cerr << options.help() << "\n";
        return exit_codes::kUsage;
    }

    const int rows = 100;

    const std::string template_type = result["template"].as<std::string>();
    if (template_type != "file" && template_type != "database") {
        std::cerr << "--template must be file or database\n";
        return exit_codes::kUsage;
    }
    const bool is_file_template     = (template_type == "file");
    const bool is_database_template = (template_type == "database");

    std::string file_format = "csv";
    if (result.count("format")) {
        file_format = result["format"].as<std::string>();
        if (file_format !=
            "csv" &&
            file_format !=
            "json" &&
            file_format !=
            "sql" &&
            file_format !=
            "Tab-Delimited" &&
            file_format != "Custom") {
            std::cerr << "Unsupported format: " << file_format << "\n";
            return exit_codes::kUsage;
        }
        if (is_database_template) { std::cerr << "Warning: --format is ignored for database templates\n"; }
    }

    const bool        has_from_database        = result.count("from-database") > 0;
    const bool        has_table                = result.count("table") > 0;
    const std::string from_database_connection = has_from_database ? result["from-database"].as<std::string>() : "";
    const std::string table_name               = has_table ? result["table"].as<std::string>() : "";

    if (has_from_database && from_database_connection.empty()) {
        std::cerr << "--from-database must not be empty\n";
        return exit_codes::kUsage;
    }
    if (has_table && table_name.empty()) {
        std::cerr << "--table must not be empty\n";
        return exit_codes::kUsage;
    }

    if (is_file_template && has_from_database) {
        std::cerr << "Warning: --from-database is ignored for file templates\n";
    }
    if (is_file_template && has_table && file_format != "sql") {
        std::cerr << "Warning: --table is ignored for non-sql file templates\n";
    }
    if (is_database_template && (has_from_database != has_table)) {
        std::cerr << "Warning: --from-database and --table must be provided together to infer fields\n";
    }

    const bool should_infer = is_database_template && has_from_database && has_table;

    const std::string default_connection =
        "odbc://DRIVER={MySQL ODBC 8.0 Driver};SERVER=127.0.0.1;PORT=3306;DATABASE=example_db;UID=user;PWD=password;";
    const std::string default_table = "generated_data";

    const std::string output_connection =
        should_infer ? from_database_connection : (has_from_database ? from_database_connection : default_connection);
    const std::string output_table = has_table ? table_name : default_table;

    OrderedJson output = OrderedJson::object();
    output["$schema"]  = "./schema/data-generator.schema.json";
    output["rows"]     = rows;

    OrderedJson output_section = OrderedJson::object();
    output_section["type"]     = template_type;

    OrderedJson fields = OrderedJson::array();

    if (is_database_template) {
        OrderedJson database                = OrderedJson::object();
        database["connection"]              = output_connection;
        database["table"]                   = output_table;
        database["insert_mode"]             = "auto";
        database["batch_size"]              = 1000;
        database["queue_size"]              = 1024;
        database["threads"]                 = 2;
        database["transaction_mode"]        = "per-batch";
        database["error_policy"]            = "stop";
        database["rate_limit_rows_per_sec"] = 20000;
        output_section["database"]          = std::move(database);

        if (should_infer) {
            database::DbUrl parsed_url;
            std::string     error;
            if (!database::parse_db_connection(from_database_connection, &parsed_url, &error)) {
                std::cerr << "Invalid --from-database: " << error << "\n";
                return exit_codes::kUsage;
            }

            auto driver = database::make_database_driver(parsed_url.type);
            if (!driver) {
                std::cerr << "Unsupported database connection type\n";
                return exit_codes::kUsage;
            }
            if (!driver->connect(parsed_url, &error)) {
                std::cerr << "Failed to connect database: " << error << "\n";
                return exit_codes::kRuntimeFailure;
            }

            database::TableMetadata metadata;
            if (!driver->get_table_metadata(table_name, &metadata, &error)) {
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
    } else {
        OrderedJson file         = OrderedJson::object();
        file["format"]           = file_format;
        OrderedJson file_options = OrderedJson::object();
        if (file_format == "csv") {
            file_options["header"]      = true;
            file_options["line_ending"] = "LF";
        } else if (file_format == "json") {
            file_options["array"]        = true;
            file_options["include_null"] = true;
        } else if (file_format == "sql") {
            file_options["table"]        = output_table;
            file_options["create_table"] = false;
        } else if (file_format == "Tab-Delimited") {
            file_options["header"]      = true;
            file_options["line_ending"] = "LF";
        } else if (file_format == "Custom") {
            file_options["delimiter"]   = ",";
            file_options["quote"]       = "\"";
            file_options["header"]      = true;
            file_options["line_ending"] = "LF";
        }
        if (!file_options.empty()) { file["options"] = std::move(file_options); }
        output_section["file"] = std::move(file);
    }

    if (fields.empty()) {
        const Json root_template = config::build_project_template(rows, file_format);
        fields                   = build_fields_from_template(root_template);
    }

    output["output"] = std::move(output_section);
    output["fields"] = std::move(fields);

    const std::string path    = result["config"].as<std::string>();
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
