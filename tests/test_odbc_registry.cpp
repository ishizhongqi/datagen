/// @file test_odbc_registry.cpp

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "output/database/odbc_registry.h"

using datagen::database::OdbcDriverInfo;
using datagen::database::list_odbc_drivers;

TEST(OdbcRegistryTest, HandlesNullPointerAndListsDrivers) {
    std::string error;
    EXPECT_FALSE(list_odbc_drivers(nullptr, &error));
    EXPECT_FALSE(error.empty());

    std::vector<OdbcDriverInfo> drivers;
    error.clear();
    EXPECT_TRUE(list_odbc_drivers(&drivers, &error)) << error;
}
