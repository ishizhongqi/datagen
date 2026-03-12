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

#include <sstream>

namespace data_generator::database {

namespace {

constexpr std::size_t kBufferSize = 4096;

std::string parse_odbc_diagnostics(const SQLSMALLINT handle_type, const SQLHANDLE handle) {
    std::ostringstream message;
    SQLSMALLINT        rec_number = 1;

    while (true) {
        SQLCHAR     state[6] = {0};
        SQLINTEGER  native_error = 0;
        SQLCHAR     text[kBufferSize] = {0};
        SQLSMALLINT text_length = 0;
        const SQLRETURN rc = SQLGetDiagRec(
            handle_type,
            handle,
            rec_number,
            state,
            &native_error,
            text,
            static_cast<SQLSMALLINT>(sizeof(text)),
            &text_length
        );
        if (rc == SQL_NO_DATA) { break; }
        if (!SQL_SUCCEEDED(rc)) {
            if (rec_number == 1) { return "ODBC operation failed (diagnostics unavailable)"; }
            break;
        }

        if (rec_number > 1) { message << " | "; }
        message << "[" << reinterpret_cast<const char*>(state) << "] "
                << reinterpret_cast<const char*>(text);
        ++rec_number;
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

    SQLHENV env = SQL_NULL_HENV;
    SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
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

    SQLCHAR      desc[kBufferSize] = {0};
    SQLSMALLINT  desc_length = 0;
    SQLCHAR      attrs[kBufferSize] = {0};
    SQLSMALLINT  attrs_length = 0;
    SQLUSMALLINT direction = SQL_FETCH_FIRST;

    while (true) {
        rc = SQLDrivers(
            env,
            direction,
            desc,
            static_cast<SQLSMALLINT>(sizeof(desc)),
            &desc_length,
            attrs,
            static_cast<SQLSMALLINT>(sizeof(attrs)),
            &attrs_length
        );

        if (rc == SQL_NO_DATA) { break; }
        if (!SQL_SUCCEEDED(rc)) {
            if (error_message) { *error_message = parse_odbc_diagnostics(SQL_HANDLE_ENV, env); }
            (void)SQLFreeHandle(SQL_HANDLE_ENV, env);
            return false;
        }

        drivers->push_back(OdbcDriverInfo{
            .name = reinterpret_cast<const char*>(desc),
            .attributes = reinterpret_cast<const char*>(attrs),
        });
        direction = SQL_FETCH_NEXT;
    }

    (void)SQLFreeHandle(SQL_HANDLE_ENV, env);
    return true;
}

}  // namespace data_generator::database
