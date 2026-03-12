// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file db_url_parser.cpp

#include "output/database/db_url_parser.h"

#include <cctype>
#include <cstring>
#include <iomanip>
#include <regex>
#include <sstream>

#include "utils/env_utils.h"

namespace data_generator::database {

namespace {

constexpr const char* kDefaultMysqlOdbcDriver = "MySQL ODBC 8.0 Unicode Driver";
constexpr const char* kDefaultPostgresqlOdbcDriver = "PostgreSQL Unicode";
constexpr const char* kDefaultOracleOdbcDriver = "Oracle ODBC Driver";

const std::regex kOdbcPrefixRegex(R"(^odbc:([a-zA-Z0-9_+.-]+):(.*)$)");
const std::regex kLegacyDbUrlRegex(
    R"(^([a-zA-Z0-9_+.-]+)://([^:/?#]+)(?::([^@/?#]*))?@([^:/?#]+)(?::([0-9]+))?/([^?#]+)$)"
);

DbType parse_db_type(const std::string& type) {
    if (type == "mysql") { return DbType::Mysql; }
    if (type == "postgresql" || type == "postgres") { return DbType::Postgresql; }
    if (type == "oracle") { return DbType::Oracle; }
    if (type == "sqlite") { return DbType::Sqlite; }
    return DbType::Unknown;
}

std::string db_type_to_url_scheme(const DbType type) {
    switch (type) {
    case DbType::Mysql     : return "mysql";
    case DbType::Postgresql: return "postgresql";
    case DbType::Oracle    : return "oracle";
    case DbType::Sqlite    : return "sqlite";
    case DbType::Unknown   : return "";
    }
    return "";
}

std::string percent_decode(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.size());

    for (std::size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            const char hi = encoded[i + 1];
            const char lo = encoded[i + 2];
            if (std::isxdigit(static_cast<unsigned char>(hi)) && std::isxdigit(static_cast<unsigned char>(lo))) {
                const int value = std::stoi(encoded.substr(i + 1, 2), nullptr, 16);
                decoded.push_back(static_cast<char>(value));
                i += 2;
                continue;
            }
        }
        decoded.push_back(encoded[i]);
    }
    return decoded;
}

std::string percent_encode(const std::string& plain) {
    std::ostringstream oss;
    for (const char ch_char : plain) {
        const auto ch = static_cast<unsigned char>(ch_char);
        if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            oss << static_cast<char>(ch);
        } else {
            oss << "%" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(ch) << std::nouppercase << std::dec;
        }
    }
    return oss.str();
}

std::string to_lower_ascii(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string odbc_driver_name_for_db_type(const DbType type) {
    switch (type) {
    case DbType::Mysql:
        return utils::get_env_or_default("DATA_GENERATOR_ODBC_DRIVER_MYSQL", kDefaultMysqlOdbcDriver);
    case DbType::Postgresql:
        return utils::get_env_or_default("DATA_GENERATOR_ODBC_DRIVER_POSTGRESQL", kDefaultPostgresqlOdbcDriver);
    case DbType::Oracle:
        return utils::get_env_or_default("DATA_GENERATOR_ODBC_DRIVER_ORACLE", kDefaultOracleOdbcDriver);
    case DbType::Sqlite:
    case DbType::Unknown:
        return "";
    }
    return "";
}

std::string build_odbc_connection_string(
    const DbType       type,
    const std::string& user,
    const std::string& password,
    const std::string& host,
    const int          port,
    const std::string& database
) {
    const std::string driver = odbc_driver_name_for_db_type(type);
    if (driver.empty()) { return ""; }

    std::ostringstream oss;
    oss << "DRIVER={" << driver << "};";
    if (type == DbType::Oracle) {
        oss << "DBQ=" << host << ":" << port << "/" << database << ";";
    } else {
        oss << "SERVER=" << host << ";";
        oss << "PORT=" << port << ";";
        oss << "DATABASE=" << database << ";";
    }
    oss << "UID=" << user << ";";
    oss << "PWD=" << password << ";";
    return oss.str();
}

DbType infer_db_type_from_odbc_connection_string(const std::string& raw) {
    const std::string lower = to_lower_ascii(raw);
    if (lower.find("mysql") != std::string::npos) { return DbType::Mysql; }
    if (lower.find("postgres") != std::string::npos) { return DbType::Postgresql; }
    if (lower.find("oracle") != std::string::npos) { return DbType::Oracle; }
    return DbType::Unknown;
}

bool parse_odbc_prefixed_string(const std::string& raw, DbUrl* parsed, std::string* error_message) {
    std::smatch match;
    if (!std::regex_match(raw, match, kOdbcPrefixRegex)) {
        if (error_message) {
            *error_message = "invalid ODBC string format, expected odbc:<mysql|postgresql|oracle>:<connection_string>";
        }
        return false;
    }

    const DbType type = parse_db_type(to_lower_ascii(match[1].str()));
    if (type == DbType::Unknown) {
        if (error_message) { *error_message = "unsupported ODBC database type: " + match[1].str(); }
        return false;
    }

    parsed->type = type;
    parsed->raw = raw;
    parsed->odbc_connection_string = match[2].str();
    return true;
}

bool parse_sqlite_url(const std::string& url, DbUrl* parsed, std::string* error_message) {
    constexpr const char* kPrefix = "sqlite:";
    if (url.rfind(kPrefix, 0) != 0) { return false; }

    std::string path = url.substr(std::strlen(kPrefix));
    if (path.rfind("//", 0) == 0) {
        path = path.substr(2);
    }
    if (path.empty()) {
        if (error_message) { *error_message = "sqlite url missing path"; }
        return false;
    }

    if (path == "memory" || path == ":memory:") { path = ":memory:"; }
    path = percent_decode(path);

    parsed->type = DbType::Sqlite;
    parsed->raw = url;
    parsed->database = path;
    return true;
}

}  // namespace

bool parse_db_url(const std::string& url, DbUrl* parsed, std::string* error_message) {
    if (!parsed) {
        if (error_message) { *error_message = "output pointer is null"; }
        return false;
    }

    if (url.rfind("sqlite:", 0) == 0) {
        return parse_sqlite_url(url, parsed, error_message);
    }

    if (url.rfind("odbc:", 0) == 0) {
        DbUrl odbc_result;
        if (!parse_odbc_prefixed_string(url, &odbc_result, error_message)) { return false; }
        *parsed = std::move(odbc_result);
        return true;
    }

    if (url.find("://") == std::string::npos) {
        const DbType inferred_type = infer_db_type_from_odbc_connection_string(url);
        if (inferred_type == DbType::Unknown) {
            if (error_message) {
                *error_message =
                    "unsupported database string. Use sqlite:<path> or legacy URL scheme://user:pass@host:port/db "
                    "or ODBC odbc:<type>:<connection_string>";
            }
            return false;
        }
        parsed->type = inferred_type;
        parsed->raw = url;
        parsed->odbc_connection_string = url;
        return true;
    }

    // Legacy URL format: scheme://user:pass@host:port/dbname
    std::smatch match;
    if (!std::regex_match(url, match, kLegacyDbUrlRegex)) {
        if (error_message) {
            *error_message =
                "invalid database URL format, expected scheme://user:pass@host:port/dbname "
                "or ODBC odbc:<type>:<connection_string>";
        }
        return false;
    }

    DbUrl result;
    result.raw      = url;
    result.type     = parse_db_type(match[1].str());
    result.username = percent_decode(match[2].str());
    result.password = percent_decode(match[3].str());
    result.host     = percent_decode(match[4].str());
    result.database = percent_decode(match[6].str());

    if (match[5].matched) {
        try {
            result.port = std::stoi(match[5].str());
        } catch (...) {
            if (error_message) { *error_message = "invalid port in URL"; }
            return false;
        }
    } else {
        switch (result.type) {
        case DbType::Mysql     : result.port = 3306; break;
        case DbType::Postgresql: result.port = 5432; break;
        case DbType::Oracle    : result.port = 1521; break;
        case DbType::Sqlite    : result.port = 0; break;
        case DbType::Unknown   : result.port = 0; break;
        }
    }

    if (result.type == DbType::Unknown || result.type == DbType::Sqlite) {
        if (error_message) {
            *error_message = "unsupported database type in URL: " + match[1].str();
        }
        return false;
    }

    result.odbc_connection_string = build_odbc_connection_string(
        result.type,
        result.username,
        result.password,
        result.host,
        result.port,
        result.database
    );

    if (result.odbc_connection_string.empty()) {
        if (error_message) { *error_message = "failed to build ODBC connection string from URL"; }
        return false;
    }

    *parsed = std::move(result);
    return true;
}

std::string build_db_url(
    const DbType              type,
    const std::string&        user,
    const std::string&        password,
    const std::string&        host,
    const std::optional<int>& port,
    const std::string&        database
) {
    int resolved_port = 0;
    if (port.has_value()) {
        resolved_port = *port;
    } else if (type == DbType::Mysql) {
        resolved_port = 3306;
    } else if (type == DbType::Postgresql) {
        resolved_port = 5432;
    } else if (type == DbType::Oracle) {
        resolved_port = 1521;
    } else if (type == DbType::Sqlite) {
        if (database == ":memory:") { return "sqlite::memory:"; }
        return "sqlite:" + percent_encode(database);
    }

    const std::string connection_string = build_odbc_connection_string(
        type,
        percent_decode(percent_encode(user)),
        percent_decode(percent_encode(password)),
        percent_decode(percent_encode(host)),
        resolved_port,
        percent_decode(percent_encode(database))
    );
    if (connection_string.empty()) { return ""; }
    return "odbc:" + db_type_to_url_scheme(type) + ":" + connection_string;
}

}  // namespace data_generator::database
