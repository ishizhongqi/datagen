// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file datetime_generators.cpp

#include "datetime_generators.h"

#include <faker/faker.h>

#include <string>

#include "generators/core/array_parser.h"
#include "generators/core/generator_base.h"
#include "generators/core/generator_registry.h"
#include "generators/core/override_rules.h"

namespace data_generator::generator {

namespace {

class DateGenerator : public IGenerator {
public:
    DateGenerator(
        std::string             start_date,
        std::string             end_date,
        const faker::DaysOfWeek days_of_week,
        OverrideState           overrides
    ) :
        start_date_(std::move(start_date)),
        end_date_(std::move(end_date)),
        days_of_week_(days_of_week),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::datetime::date(start_date_, end_date_, days_of_week_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    std::string       start_date_;
    std::string       end_date_;
    faker::DaysOfWeek days_of_week_;
    OverrideState     overrides_;
};

class TimeGenerator : public IGenerator {
public:
    TimeGenerator(std::string start_time, std::string end_time, OverrideState overrides) :
        start_time_(std::move(start_time)), end_time_(std::move(end_time)), overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::datetime::time(start_time_, end_time_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    std::string   start_time_;
    std::string   end_time_;
    OverrideState overrides_;
};

class DateTimeGenerator : public IGenerator {
public:
    DateTimeGenerator(
        std::string       start_date,
        std::string       end_date,
        std::string       start_time,
        std::string       end_time,
        faker::DaysOfWeek days_of_week,
        OverrideState     overrides
    ) :
        start_date_(std::move(start_date)),
        end_date_(std::move(end_date)),
        start_time_(std::move(start_time)),
        end_time_(std::move(end_time)),
        days_of_week_(days_of_week),
        overrides_(std::move(overrides)) {}

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return faker::datetime::datetime(start_date_, end_date_, start_time_, end_time_, days_of_week_);
    }

    void next() override {
        next_row(overrides_);
    }

private:
    std::string       start_date_;
    std::string       end_date_;
    std::string       start_time_;
    std::string       end_time_;
    faker::DaysOfWeek days_of_week_;
    OverrideState     overrides_;
};

}  // namespace

void register_datetime_generators(GeneratorRegistry& registry) {
    registry.register_generator("date", [](const Json& filed) {
        const Json&       config       = filed.at("config");
        const auto        overrides    = parse_overrides(filed);
        const auto        days_of_week = parse_days_of_week(config);
        const std::string start_date   = config.value("start_date", "1970-01-01");
        const std::string end_date     = config.value("end_date", "2050-12-31");
        return std::make_unique<DateGenerator>(start_date, end_date, days_of_week, overrides);
    });

    registry.register_generator("time", [](const Json& filed) {
        const Json&       config     = filed.at("config");
        const auto        overrides  = parse_overrides(filed);
        const std::string start_time = config.value("start_time", "00:00:00");
        const std::string end_time   = config.value("end_time", "23:59:59");
        return std::make_unique<TimeGenerator>(start_time, end_time, overrides);
    });

    registry.register_generator("datetime", [](const Json& filed) {
        const Json&       config       = filed.at("config");
        const auto        overrides    = parse_overrides(filed);
        const auto        days_of_week = parse_days_of_week(config);
        const std::string start_date   = config.value("start_date", "1970-01-01");
        const std::string end_date     = config.value("end_date", "2050-12-31");
        const std::string start_time   = config.value("start_time", "00:00:00");
        const std::string end_time     = config.value("end_time", "23:59:59");
        return std::make_unique<DateTimeGenerator>(start_date, end_date, start_time, end_time, days_of_week, overrides);
    });
}

}  // namespace data_generator::generator
