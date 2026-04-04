// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file odbc_registry.cpp

#include "output/database/odbc_registry.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

#include <limits>
#include <sstream>

namespace data_generator::database {

namespace {

constexpr std::size_t kBufferSize = 4096;

std::string parse_odbc_diagnostics(const SQLSMALLINT handle_type, SQLHANDLE handle) {
    std::ostringstream message;

    for (int rec_number = 1; rec_number <= std::numeric_limits<SQLSMALLINT>::max(); ++rec_number) {
        SQLCHAR         state[6]          = {};
        SQLINTEGER      native_error      = 0;
        SQLCHAR         text[kBufferSize] = {};
        SQLSMALLINT     text_length       = 0;
        const SQLRETURN rc                = SQLGetDiagRec(
            handle_type,
            handle,
            static_cast<SQLSMALLINT>(rec_number),
            state,
            &native_error,
            text,
            sizeof(text),
            &text_length
        );
        if (rc == SQL_NO_DATA) { break; }
        if (!SQL_SUCCEEDED(rc)) {
            if (rec_number == 1) { return "ODBC operation failed (diagnostics unavailable)"; }
            break;
        }

        if (rec_number > 1) { message << " | "; }
        message << "[" << reinterpret_cast<const char*>(state) << "] " << reinterpret_cast<const char*>(text);
    }

    const std::string result = message.str();
    return result.empty() ? "ODBC operation failed" : result;
}

}  // namespace

bool list_odbc_drivers(std::vector<OdbcDriverInfo>* drivers, std::string* error_message) {
    if (!drivers) {
        if (error_message) { *error_message = "drivers output pointer is null"; }
        return false;
    }
    drivers->clear();

    SQLHENV   env = SQL_NULL_HENV;
    SQLRETURN rc  = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (!SQL_SUCCEEDED(rc)) {
        if (error_message) { *error_message = "failed to allocate ODBC environment"; }
        return false;
    }

    rc = SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    if (!SQL_SUCCEEDED(rc)) {
        if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_ENV, env); }
        (void)SQLFreeHandle(SQL_HANDLE_ENV, env);
        return false;
    }

    SQLCHAR     desc[kBufferSize]  = {};
    SQLSMALLINT desc_length        = 0;
    SQLCHAR     attrs[kBufferSize] = {};
    SQLSMALLINT attrs_length       = 0;

    for (SQLUSMALLINT direction = SQL_FETCH_FIRST; rc != SQL_NO_DATA; direction = SQL_FETCH_NEXT) {
        rc = SQLDrivers(env, direction, desc, sizeof(desc), &desc_length, attrs, sizeof(attrs), &attrs_length);

        if (rc == SQL_NO_DATA) { break; }
        if (!SQL_SUCCEEDED(rc)) {
            if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_ENV, env); }
            (void)SQLFreeHandle(SQL_HANDLE_ENV, env);
            return false;
        }

        drivers->push_back(
            OdbcDriverInfo{
                .name       = reinterpret_cast<const char*>(desc),
                .attributes = reinterpret_cast<const char*>(attrs),
            }
        );
    }

    (void)SQLFreeHandle(SQL_HANDLE_ENV, env);
    return true;
}

}  // namespace data_generator::database
