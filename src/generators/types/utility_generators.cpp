// Copyright (c) 2026 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file utility_generators.cpp

#include "utility_generators.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "generators/core/generator_base.h"
#include "generators/core/generator_registry.h"
#include "generators/core/override_rules.h"

namespace data_generator::generator {

namespace {

using PatternSize = std::string::size_type;

class BooleanGenerator : public IGenerator {
public:
    BooleanGenerator(const double true_percentage, OverrideState overrides) :
        distribution_(true_percentage / 100.0), overrides_(std::move(overrides)) {
        if (true_percentage < 0.0 || true_percentage > 100.0) {
            throw std::invalid_argument("boolean true_percentage must be between 0 and 100");
        }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return distribution_(rng_) ? "true" : "false";
    }

    void next() override {
        next_row(overrides_);
    }

private:
    std::bernoulli_distribution distribution_;
    std::mt19937_64             rng_{std::random_device{}()};
    OverrideState               overrides_;
};

class SequenceGenerator : public IGenerator {
public:
    SequenceGenerator(int64_t start, int64_t end, int64_t step, bool circle, OverrideState overrides) :
        current_(start), start_(start), end_(end), step_(step), circle_(circle), overrides_(std::move(overrides)) {
        if (step_ == 0) { throw std::invalid_argument("sequence step must not be 0"); }
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        return std::to_string(current_);
    }

    void next() override {
        next_row(overrides_);
        const int64_t next_value = current_ + step_;
        if ((step_ > 0 && next_value > end_) || (step_ < 0 && next_value < end_)) {
            if (circle_) {
                current_ = start_;
            } else {
                current_ = end_;
            }
        } else {
            current_ = next_value;
        }
    }

private:
    int64_t       current_;
    int64_t       start_;
    int64_t       end_;
    int64_t       step_;
    bool          circle_;
    OverrideState overrides_;
};

struct RegexToken {
    std::vector<char> charset;
    int               min_repeat = 1;
    int               max_repeat = 1;
};

class RegexGenerator : public IGenerator {
public:
    RegexGenerator(std::string pattern, OverrideState overrides) :
        pattern_(std::move(pattern)), overrides_(std::move(overrides)) {
        parse_pattern();
    }

    std::string generate() override {
        if (auto overridden = apply_override(overrides_)) { return *overridden; }
        std::string result;
        result.reserve(32);
        for (const auto& token : tokens_) {
            const int count = pick_repeat(token.min_repeat, token.max_repeat);
            for (int i = 0; i < count; ++i) { result.push_back(pick_char(token.charset)); }
        }
        return result;
    }

    void next() override {
        next_row(overrides_);
    }

private:
    int pick_repeat(const int min_repeat, const int max_repeat) {
        if (min_repeat == max_repeat) { return min_repeat; }
        std::uniform_int_distribution<int> dist(min_repeat, max_repeat);
        return dist(rng_);
    }

    char pick_char(const std::vector<char>& charset) {
        if (charset.empty()) { return '\0'; }
        std::uniform_int_distribution<std::size_t> dist(0, charset.size() - 1);
        return charset[dist(rng_)];
    }

    static bool is_quantifier_start(const char ch) {
        return ch == '{' || ch == '?' || ch == '*' || ch == '+';
    }

    void apply_quantifier(RegexToken& token) {
        if (pos_ >= pattern_.size()) { return; }
        const char ch = pattern_[pos_];
        if (!is_quantifier_start(ch)) { return; }

        if (ch == '?') {
            token.min_repeat = 0;
            token.max_repeat = 1;
            ++pos_;
            return;
        }
        if (ch == '*') {
            token.min_repeat = 0;
            token.max_repeat = 10;
            ++pos_;
            return;
        }
        if (ch == '+') {
            token.min_repeat = 1;
            token.max_repeat = 10;
            ++pos_;
            return;
        }
        if (ch == '{') {
            ++pos_;
            int min_repeat = 0;
            int max_repeat = 0;
            if (pos_ >= pattern_.size() || !std::isdigit(static_cast<unsigned char>(pattern_[pos_]))) {
                throw std::invalid_argument("Invalid regular_expression quantifier");
            }
            while (pos_ < pattern_.size() && std::isdigit(static_cast<unsigned char>(pattern_[pos_]))) {
                min_repeat = min_repeat * 10 + (pattern_[pos_] - '0');
                ++pos_;
            }
            if (pos_ < pattern_.size() && pattern_[pos_] == ',') {
                ++pos_;
                if (pos_ < pattern_.size() && std::isdigit(static_cast<unsigned char>(pattern_[pos_]))) {
                    while (pos_ < pattern_.size() && std::isdigit(static_cast<unsigned char>(pattern_[pos_]))) {
                        max_repeat = max_repeat * 10 + (pattern_[pos_] - '0');
                        ++pos_;
                    }
                } else {
                    max_repeat = min_repeat + 10;
                }
            } else {
                max_repeat = min_repeat;
            }
            if (pos_ >= pattern_.size() || pattern_[pos_] != '}') {
                throw std::invalid_argument("Invalid regular_expression quantifier");
            }
            ++pos_;
            if (max_repeat < min_repeat) { throw std::invalid_argument("Invalid regular_expression quantifier range"); }
            token.min_repeat = min_repeat;
            token.max_repeat = max_repeat;
        }
    }

    std::vector<char> parse_charset() {
        std::vector<char> charset;
        if (pattern_[pos_] != '[') { return charset; }
        ++pos_;
        bool negate = false;
        if (pos_ < pattern_.size() && pattern_[pos_] == '^') {
            negate = true;
            ++pos_;
        }
        while (pos_ < pattern_.size() && pattern_[pos_] != ']') {
            const char start = pattern_[pos_++];
            if (pos_ + static_cast<PatternSize>(1) < pattern_.size() && pattern_[pos_] == '-') {
                ++pos_;
                const char end = pattern_[pos_++];
                for (char c = start; c <= end; ++c) { charset.push_back(c); }
            } else {
                charset.push_back(start);
            }
        }
        if (pos_ >= pattern_.size() || pattern_[pos_] != ']') {
            throw std::invalid_argument("Unclosed character class in regular_expression");
        }
        ++pos_;
        if (negate) {
            std::vector<char> negated;
            for (char c = 32; c < 127; ++c) {
                if (std::find(charset.begin(), charset.end(), c) == charset.end()) { negated.push_back(c); }
            }
            charset.swap(negated);
        }
        return charset;
    }

    static std::vector<char> escape_charset(const char ch) {
        if (ch == 'd') {
            std::vector<char> digits;
            for (char c = '0'; c <= '9'; ++c) { digits.push_back(c); }
            return digits;
        }
        if (ch == 'w') {
            std::vector<char> chars;
            for (char c = '0'; c <= '9'; ++c) { chars.push_back(c); }
            for (char c = 'A'; c <= 'Z'; ++c) { chars.push_back(c); }
            for (char c = 'a'; c <= 'z'; ++c) { chars.push_back(c); }
            chars.push_back('_');
            return chars;
        }
        if (ch == 's') { return {' '}; }
        return {ch};
    }

    void parse_pattern() {
        while (pos_ < pattern_.size()) {
            const char ch = pattern_[pos_];
            if (ch == '\\') {
                ++pos_;
                if (pos_ >= pattern_.size()) { throw std::invalid_argument("Invalid escape in regular_expression"); }
                RegexToken token;
                token.charset = escape_charset(pattern_[pos_++]);
                apply_quantifier(token);
                tokens_.push_back(std::move(token));
                continue;
            }
            if (ch == '[') {
                RegexToken token;
                token.charset = parse_charset();
                apply_quantifier(token);
                tokens_.push_back(std::move(token));
                continue;
            }
            if (is_quantifier_start(ch)) { throw std::invalid_argument("Dangling quantifier in regular_expression"); }
            ++pos_;
            RegexToken token;
            token.charset = {ch};
            apply_quantifier(token);
            tokens_.push_back(std::move(token));
        }
    }

    std::string             pattern_;
    std::vector<RegexToken> tokens_;
    PatternSize             pos_ = 0;
    std::mt19937_64         rng_{std::random_device{}()};
    OverrideState           overrides_;
};

}  // namespace

void register_utility_generators(GeneratorRegistry& registry) {
    registry.register_generator("boolean", [](const Json& filed) {
        const Json& config          = filed.at("config");
        const auto  overrides       = parse_overrides(filed);
        const double true_percentage = config.value("true_percentage", 50.0);
        return std::make_unique<BooleanGenerator>(true_percentage, overrides);
    });

    registry.register_generator("sequence", [](const Json& filed) {
        const Json&   config    = filed.at("config");
        const auto    overrides = parse_overrides(filed);
        const int64_t start     = config.value("start", static_cast<int64_t>(1));
        const int64_t end       = config.value("end", static_cast<int64_t>(100));
        const int64_t step      = config.value("step", static_cast<int64_t>(1));
        const bool    circle    = config.value("circle", true);
        return std::make_unique<SequenceGenerator>(start, end, step, circle, overrides);
    });

    registry.register_generator("regular_expression", [](const Json& filed) {
        const Json& config    = filed.at("config");
        const auto  overrides = parse_overrides(filed);
        if (!config.contains("pattern")) { throw std::invalid_argument("regular_expression requires pattern"); }
        const std::string pattern = config.at("pattern").get<std::string>();
        return std::make_unique<RegexGenerator>(pattern, overrides);
    });
}

}  // namespace data_generator::generator
