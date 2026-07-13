/// @file test_configuration_edge.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

#include "config/configuration.h"

using namespace datagen;

namespace {

bool has_issue(
    const std::vector<config::ValidationIssue>& issues,
    const std::string&                          path,
    const std::string&                          message_substring
) {
    for (const auto& issue : issues) {
        if (issue.path == path && issue.message.find(message_substring) != std::string::npos) {
            return true;
        }
    }
    return false;
}

TEST(ConfigurationEdgeTest, RootAndOutputValidationBranches) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::array(),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(issues.empty());

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":"x","output":1,"fields":[]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(issues.empty());

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output":{"type":"bad"},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":1,"file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output":{"type":"file","file":{"format":"bad"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"},"unknown":1},"unknown_root":1,"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2},"unknown_field":1}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_TRUE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"$schema":"./schema/datagen.schema.json","rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":0,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":{}})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"date","unique":true,"config":{"start_date":"2024-01-01","end_date":"2024-01-31"}}]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
}

TEST(ConfigurationEdgeTest, FieldValidationBranches) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[1]})json"),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":1,"generator":2,"config":3}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2},"unique":"x"}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2},"group":123}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2,"bad":1},"default_value":1}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));

    EXPECT_TRUE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv"}},"fields":[{"name":"f","generator":"date","config":{"end_date":"2024-01-01"}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
}

TEST(ConfigurationEdgeTest, AllowMissingOutputModeSucceedsWithDefaults) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    EXPECT_TRUE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::AllowMissingOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_EQ(cfg.rows, 100);
    EXPECT_EQ(cfg.output.type, config::OutputType::File);
    EXPECT_EQ(cfg.output.file.format, config::OutputFormat::Csv);
}

TEST(ConfigurationEdgeTest, ApplyOverridesAndFormatRoundtrip) {
    EXPECT_EQ(config::parse_output_format("csv"), config::OutputFormat::Csv);
    EXPECT_EQ(config::parse_output_format("json"), config::OutputFormat::JsonFormat);
    EXPECT_EQ(config::parse_output_format("sql"), config::OutputFormat::Sql);
    EXPECT_EQ(config::parse_output_format("Tab-Delimited"), config::OutputFormat::TabDelimited);
    EXPECT_EQ(config::parse_output_format("Custom"), config::OutputFormat::Custom);
    EXPECT_FALSE(config::parse_output_format("x").has_value());

    EXPECT_EQ(config::output_format_to_string(config::OutputFormat::Csv), "csv");
    EXPECT_EQ(config::output_format_to_string(config::OutputFormat::JsonFormat), "json");
    EXPECT_EQ(config::output_format_to_string(config::OutputFormat::Sql), "sql");
    EXPECT_EQ(config::output_format_to_string(config::OutputFormat::TabDelimited), "Tab-Delimited");
    EXPECT_EQ(config::output_format_to_string(config::OutputFormat::Custom), "Custom");
    EXPECT_EQ(config::output_format_to_string(static_cast<config::OutputFormat>(99)), "csv");

    EXPECT_EQ(config::parse_output_type("file"), config::OutputType::File);
    EXPECT_EQ(config::parse_output_type("database"), config::OutputType::Database);
    EXPECT_FALSE(config::parse_output_type("x").has_value());

    config::GenerationConfig cfg;
    cfg.rows = 10;
    cfg.output.file.format = config::OutputFormat::Csv;
    cfg.output.database.table = "t";

    config::CliOverrides overrides;
    overrides.rows = 20;
    overrides.format = config::OutputFormat::Sql;
    overrides.table_name = std::string("t2");
    overrides.type = config::OutputType::Database;
    overrides.output_path = std::string("out.csv");
    overrides.database_connection = std::string("sqlite://:memory:");
    apply_cli_overrides(&cfg, overrides);
    EXPECT_EQ(cfg.rows, 20);
    EXPECT_EQ(cfg.output.file.format, config::OutputFormat::Sql);
    EXPECT_EQ(cfg.output.database.table, "t2");
    EXPECT_EQ(cfg.output.file.sql.table, "t2");
    EXPECT_EQ(cfg.output.type, config::OutputType::Database);
    EXPECT_EQ(cfg.output.file.path, "out.csv");
    EXPECT_EQ(cfg.output.database.connection, "sqlite://:memory:");
}

TEST(ConfigurationEdgeTest, DatabaseOutputParsingAndHelperRoundtripBranches) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;

    EXPECT_TRUE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":2,"output":{"type":"database","database":{"connection":"sqlite:///tmp/data.db","table":"t_data","mode":"safe","write_mode":"truncate","transaction_mode":"none","error_policy":"rollback","advanced":{"insert_mode":"load","batch_size":8,"queue_size":16,"threads":3,"rate_limit_rows_per_sec":32}}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(issues.empty());
    EXPECT_EQ(cfg.output.type, config::OutputType::Database);
    EXPECT_EQ(cfg.output.database.connection, "sqlite:///tmp/data.db");
    EXPECT_EQ(cfg.output.database.table, "t_data");
    EXPECT_EQ(cfg.output.database.mode, config::DatabaseMode::Safe);
    EXPECT_EQ(cfg.output.database.write_mode, config::WriteMode::Truncate);
    EXPECT_EQ(cfg.output.database.insert_mode, config::InsertMode::Load);
    EXPECT_EQ(cfg.output.database.batch_size, 8);
    EXPECT_EQ(cfg.output.database.queue_size, 16);
    EXPECT_EQ(cfg.output.database.db_threads, 3);
    EXPECT_EQ(cfg.output.database.transaction_mode, config::TransactionMode::None);
    EXPECT_EQ(cfg.output.database.error_policy, config::ErrorPolicy::Rollback);
    EXPECT_EQ(cfg.output.database.rate_limit_rows_per_sec, 32);

    EXPECT_EQ(config::parse_database_mode("fast"), config::DatabaseMode::Fast);
    EXPECT_EQ(config::parse_database_mode("balanced"), config::DatabaseMode::Balanced);
    EXPECT_EQ(config::parse_database_mode("safe"), config::DatabaseMode::Safe);
    EXPECT_FALSE(config::parse_database_mode("bad").has_value());
    EXPECT_EQ(config::database_mode_to_string(config::DatabaseMode::Fast), "fast");
    EXPECT_EQ(config::database_mode_to_string(config::DatabaseMode::Balanced), "balanced");
    EXPECT_EQ(config::database_mode_to_string(config::DatabaseMode::Safe), "safe");

    EXPECT_EQ(config::parse_write_mode("append"), config::WriteMode::Append);
    EXPECT_EQ(config::parse_write_mode("truncate"), config::WriteMode::Truncate);
    EXPECT_EQ(config::parse_write_mode("upsert"), config::WriteMode::Upsert);
    EXPECT_FALSE(config::parse_write_mode("bad").has_value());
    EXPECT_EQ(config::write_mode_to_string(config::WriteMode::Append), "append");
    EXPECT_EQ(config::write_mode_to_string(config::WriteMode::Truncate), "truncate");
    EXPECT_EQ(config::write_mode_to_string(config::WriteMode::Upsert), "upsert");

    EXPECT_EQ(config::parse_output_type("file"), config::OutputType::File);
    EXPECT_EQ(config::parse_output_type("database"), config::OutputType::Database);
    EXPECT_FALSE(config::parse_output_type("bad").has_value());
    EXPECT_EQ(config::output_type_to_string(config::OutputType::File), "file");
    EXPECT_EQ(config::output_type_to_string(config::OutputType::Database), "database");
    EXPECT_EQ(config::output_type_to_string(static_cast<config::OutputType>(99)), "file");

    EXPECT_EQ(config::parse_line_ending("LF"), config::LineEnding::LF);
    EXPECT_EQ(config::parse_line_ending("CRLF"), config::LineEnding::CRLF);
    EXPECT_FALSE(config::parse_line_ending("bad").has_value());
    EXPECT_EQ(config::line_ending_to_string(config::LineEnding::LF), "LF");
    EXPECT_EQ(config::line_ending_to_string(config::LineEnding::CRLF), "CRLF");
    EXPECT_EQ(config::line_ending_to_string(static_cast<config::LineEnding>(99)), "LF");

    EXPECT_EQ(config::parse_transaction_mode("per-batch"), config::TransactionMode::PerBatch);
    EXPECT_EQ(config::parse_transaction_mode("per-run"), config::TransactionMode::PerRun);
    EXPECT_EQ(config::parse_transaction_mode("none"), config::TransactionMode::None);
    EXPECT_FALSE(config::parse_transaction_mode("bad").has_value());
    EXPECT_EQ(config::transaction_mode_to_string(config::TransactionMode::PerBatch), "per-batch");
    EXPECT_EQ(config::transaction_mode_to_string(config::TransactionMode::PerRun), "per-run");
    EXPECT_EQ(config::transaction_mode_to_string(config::TransactionMode::None), "none");
    EXPECT_EQ(
        config::transaction_mode_to_string(static_cast<config::TransactionMode>(99)),
        "per-batch"
    );

    EXPECT_EQ(config::parse_error_policy("stop"), config::ErrorPolicy::Stop);
    EXPECT_EQ(config::parse_error_policy("continue"), config::ErrorPolicy::Continue);
    EXPECT_EQ(config::parse_error_policy("rollback"), config::ErrorPolicy::Rollback);
    EXPECT_FALSE(config::parse_error_policy("bad").has_value());
    EXPECT_EQ(config::error_policy_to_string(config::ErrorPolicy::Stop), "stop");
    EXPECT_EQ(config::error_policy_to_string(config::ErrorPolicy::Continue), "continue");
    EXPECT_EQ(config::error_policy_to_string(config::ErrorPolicy::Rollback), "rollback");
    EXPECT_EQ(config::error_policy_to_string(static_cast<config::ErrorPolicy>(99)), "stop");
    EXPECT_EQ(config::insert_mode_to_string(static_cast<config::InsertMode>(99)), "insert");
}

TEST(ConfigurationEdgeTest, FileOutputValidationBranches) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file"},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.file", "missing required object"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":1},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.file", "must be an object"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":1}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.file.format", "must be a string"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.file.format", "missing required format"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv","options":[]}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.file.options", "must be an object"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"csv","options":{"header":"yes","line_ending":true}}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.file.options.header", "must be a boolean"));
    EXPECT_TRUE(has_issue(issues, "$.output.file.options.line_ending", "must be a string"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"Tab-Delimited","options":{"line_ending":"bad"}}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.file.options.line_ending", "must be one of"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"Custom","options":{"delimiter":7,"quote":9,"header":"no","line_ending":"bad"}}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.file.options.delimiter", "must be a string"));
    EXPECT_TRUE(has_issue(issues, "$.output.file.options.quote", "must be a string"));
    EXPECT_TRUE(has_issue(issues, "$.output.file.options.header", "must be a boolean"));
    EXPECT_TRUE(has_issue(issues, "$.output.file.options.line_ending", "must be one of"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"file","file":{"format":"sql","options":{"table":""}}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.file.options.table", "must be a non-empty string"));
}

TEST(ConfigurationEdgeTest, DatabaseOutputValidationBranches) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"database"},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.database", "missing required object"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"database","database":1},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.database", "must be an object"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"database","database":{"table":"t_data"}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.database.connection", "requires connection"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"database","database":{"connection":7,"table":false}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.database.connection", "must be a string"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.table", "must be a string"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"database","database":{"connection":"sqlite:///tmp/data.db","table":"t_data","mode":1,"write_mode":1,"transaction_mode":1,"error_policy":1,"advanced":{"insert_mode":1}}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.database.mode", "must be a string"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.write_mode", "must be a string"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.advanced.insert_mode", "must be a string"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.transaction_mode", "must be a string"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.error_policy", "must be a string"));

    EXPECT_FALSE(config::parse_generation_config(
        nlohmann::json::parse(
            R"json({"rows":1,"output":{"type":"database","database":{"connection":"sqlite:///tmp/data.db","table":"t_data","mode":"bad","write_mode":"bad","transaction_mode":"bad","error_policy":"bad","advanced":{"insert_mode":"bad","batch_size":0,"queue_size":"bad","threads":0,"rate_limit_rows_per_sec":-1}}},"fields":[{"name":"f","generator":"integer","config":{"start":1,"end":2}}]})json"
        ),
        config::ParseMode::RequireOutputSettings,
        &cfg,
        &issues
    ));
    EXPECT_TRUE(has_issue(issues, "$.output.database.mode", "must be one of"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.write_mode", "must be one of"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.advanced.insert_mode", "must be one of"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.transaction_mode", "must be one of"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.error_policy", "must be one of"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.advanced.batch_size", "must be >="));
    EXPECT_TRUE(has_issue(issues, "$.output.database.advanced.queue_size", "must be an integer"));
    EXPECT_TRUE(has_issue(issues, "$.output.database.advanced.threads", "must be >="));
    EXPECT_TRUE(has_issue(issues, "$.output.database.advanced.rate_limit_rows_per_sec", "must be >="));
}

}  // namespace
