// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

#include "database/db_url_parser.h"

#include <cctype>
#include <iomanip>
#include <regex>
#include <sstream>

namespace data_generator::database {

namespace {

DbType parse_db_type(const std::string& type) {
    if (type == "mysql") { return DbType::Mysql; }
    if (type == "postgresql" || type == "postgres") { return DbType::Postgresql; }
    if (type == "sqlite") { return DbType::Sqlite; }
    if (type == "oracle") { return DbType::Oracle; }
    return DbType::Unknown;
}

std::string db_type_to_url_scheme(const DbType type) {
    switch (type) {
    case DbType::Mysql     : return "mysql";
    case DbType::Postgresql: return "postgresql";
    case DbType::Sqlite    : return "sqlite";
    case DbType::Oracle    : return "oracle";
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
        const unsigned char ch = static_cast<unsigned char>(ch_char);
        if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            oss << static_cast<char>(ch);
        } else {
            oss << "%" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(ch) << std::nouppercase << std::dec;
        }
    }
    return oss.str();
}

}  // namespace

bool parse_db_url(const std::string& url, DbUrl* parsed, std::string* error_message) {
    if (!parsed) {
        if (error_message) { *error_message = "output pointer is null"; }
        return false;
    }

    // Format: scheme://user:pass@host:port/dbname
    static const std::regex kRegex(R"(^([a-zA-Z0-9_+.-]+)://([^:/?#]+)(?::([^@/?#]*))?@([^:/?#]+)(?::([0-9]+))?/([^?#]+)$)");
    std::smatch match;
    if (!std::regex_match(url, match, kRegex)) {
        if (error_message) {
            *error_message = "invalid database URL format, expected scheme://user:pass@host:port/dbname";
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

    if (result.type == DbType::Unknown) {
        if (error_message) {
            *error_message = "unsupported database type in URL: " + match[1].str();
        }
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
    const std::string scheme = db_type_to_url_scheme(type);
    if (scheme.empty()) { return ""; }

    std::string url = scheme + "://" + percent_encode(user) + ":" + percent_encode(password) + "@" + percent_encode(host);
    if (port.has_value()) { url += ":" + std::to_string(*port); }
    url += "/" + percent_encode(database);
    return url;
}

}  // namespace data_generator::database
