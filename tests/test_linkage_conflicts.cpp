/// @file test_linkage_conflicts.cpp

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <sstream>
#include <vector>

#include "config/configuration.h"
#include "engine/executor.h"

using namespace datagen;

namespace {

config::GenerationConfig cfg_from_json(const char* text) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;
    const auto root = nlohmann::json::parse(text);
    const bool ok = config::parse_generation_config(root, config::ParseMode::RequireOutputSettings, &cfg, &issues);
    EXPECT_TRUE(ok);
    return cfg;
}

TEST(LinkageConflictTest, PersonConflictBranches) {
    const auto cfg = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"f1","generator":"first_name","config":{"languages":["English"],"genders":["M"],"use_translation":false},"group":"person:x"},
    {"name":"f2","generator":"last_name","config":{"languages":["Japanese"],"use_translation":false},"group":"person:x"}
  ]
}
)json");
    std::ostringstream out;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 1}, out),
        std::runtime_error
    );

    const auto cfg_gender = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"f1","generator":"first_name","config":{"languages":["English"],"genders":["M"],"use_translation":false},"group":"person:g"},
    {"name":"f2","generator":"first_name","config":{"languages":["English"],"genders":["F"],"use_translation":false},"group":"person:g"}
  ]
}
)json");
    std::ostringstream out_gender;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg_gender, engine::ExecutionOptions{.requested_threads = 1}, out_gender),
        std::runtime_error
    );
}

TEST(LinkageConflictTest, PersonConflictRegionsDomainsAndUnique) {
    const auto cfg_regions = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"p1","generator":"phone_number","config":{"regions":["United States"]},"group":"person:r"},
    {"name":"p2","generator":"phone_number","config":{"regions":["China"]},"group":"person:r"}
  ]
}
)json");
    std::ostringstream out1;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg_regions, engine::ExecutionOptions{.requested_threads = 1}, out1),
        std::runtime_error
    );

    const auto cfg_domains = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"e1","generator":"email","unique":true,"config":{"languages":["English"],"domains":["a.com"]},"group":"person:d"},
    {"name":"e2","generator":"email","unique":true,"config":{"languages":["English"],"domains":["b.com"]},"group":"person:d"}
  ]
}
)json");
    std::ostringstream out2;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg_domains, engine::ExecutionOptions{.requested_threads = 1}, out2),
        std::runtime_error
    );

    const auto cfg_unique = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"e1","generator":"email","unique":true,"config":{"languages":["English"],"domains":["a.com"]},"group":"person:u"},
    {"name":"e2","generator":"email","unique":false,"config":{"languages":["English"],"domains":["a.com"]},"group":"person:u"}
  ]
}
    )json");
    std::ostringstream out3;
    EXPECT_NO_THROW((void)engine::generate_to_stream(cfg_unique, engine::ExecutionOptions{.requested_threads = 1}, out3));
}

TEST(LinkageConflictTest, PaymentConflictBranches) {
    const auto cfg = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"c1","generator":"card_type","config":{"languages":["English"],"card_types":["Visa"]},"group":"payment:x"},
    {"name":"c2","generator":"card_type","config":{"languages":["Japanese"],"card_types":["Visa"]},"group":"payment:x"}
  ]
}
)json");
    std::ostringstream out;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 1}, out),
        std::runtime_error
    );

    const auto cfg_card_types = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"n1","generator":"card_number","config":{"card_types":["Visa"]},"group":"payment:c"},
    {"name":"n2","generator":"card_number","config":{"card_types":["MasterCard"]},"group":"payment:c"}
  ]
}
)json");
    std::ostringstream out_card_types;
    EXPECT_THROW(
        (void)engine::generate_to_stream(
            cfg_card_types,
            engine::ExecutionOptions{.requested_threads = 1},
            out_card_types
        ),
        std::runtime_error
    );
}

TEST(LinkageConflictTest, PaymentConflictCardDateAndUnique) {
    const auto cfg_start = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"d1","generator":"card_date","config":{"start_month":"01/20","end_month":"12/30"},"group":"payment:s"},
    {"name":"d2","generator":"card_date","config":{"start_month":"02/20","end_month":"12/30"},"group":"payment:s"}
  ]
}
)json");
    std::ostringstream out1;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg_start, engine::ExecutionOptions{.requested_threads = 1}, out1),
        std::runtime_error
    );

    const auto cfg_end = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"d1","generator":"card_date","config":{"start_month":"01/20","end_month":"11/30"},"group":"payment:e"},
    {"name":"d2","generator":"card_date","config":{"start_month":"01/20","end_month":"12/30"},"group":"payment:e"}
  ]
}
)json");
    std::ostringstream out2;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg_end, engine::ExecutionOptions{.requested_threads = 1}, out2),
        std::runtime_error
    );

    const auto cfg_unique = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"n1","generator":"card_number","unique":true,"config":{"card_types":["Visa"]},"group":"payment:u"},
    {"name":"n2","generator":"card_number","unique":false,"config":{"card_types":["Visa"]},"group":"payment:u"}
  ]
}
    )json");
    std::ostringstream out3;
    EXPECT_NO_THROW((void)engine::generate_to_stream(cfg_unique, engine::ExecutionOptions{.requested_threads = 1}, out3));
}

TEST(LinkageConflictTest, LocationAndComputerConflictBranches) {
    const auto cfg1 = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"l1","generator":"address_line1","config":{"regions":["United States"],"use_translation":false},"group":"location:x"},
    {"name":"l2","generator":"city","config":{"regions":["China"],"use_translation":false},"group":"location:x"}
  ]
}
)json");
    std::ostringstream out1;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg1, engine::ExecutionOptions{.requested_threads = 1}, out1),
        std::runtime_error
    );

    const auto cfg2 = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"f1","generator":"file_path","config":{"operating_systems":["Windows"],"extensions":["txt"]},"group":"file:x"},
    {"name":"f2","generator":"file_name","config":{"extensions":["csv"]},"group":"file:x"}
  ]
}
)json");
    std::ostringstream out2;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg2, engine::ExecutionOptions{.requested_threads = 1}, out2),
        std::runtime_error
    );

    const auto cfg3 = cfg_from_json(R"json(
{
  "rows": 2,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"f1","generator":"file_path","config":{"operating_systems":["Windows"],"extensions":["txt"]},"group":"file:os"},
    {"name":"f2","generator":"file_path","config":{"operating_systems":["Linux"],"extensions":["txt"]},"group":"file:os"}
  ]
}
)json");
    std::ostringstream out3;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg3, engine::ExecutionOptions{.requested_threads = 1}, out3),
        std::runtime_error
    );
}

TEST(LinkageConflictTest, ComputerAndUtilityErrorBranches) {
    config::GenerationConfig cfg;
    std::vector<config::ValidationIssue> issues;

    const auto root1 = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"u","generator":"url","config":{"subdomains":["www"]}}
  ]
}
)json");
    EXPECT_TRUE(config::parse_generation_config(root1, config::ParseMode::RequireOutputSettings, &cfg, &issues));

    const auto root2 = nlohmann::json::parse(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"h","generator":"hostname","config":{"subdomains":["www"]}}
  ]
}
)json");
    EXPECT_TRUE(config::parse_generation_config(root2, config::ParseMode::RequireOutputSettings, &cfg, &issues));

    const auto cfg_invalid_url = cfg_from_json(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"u","generator":"url","config":{"subdomains":["www"],"tlds":[]}}
  ]
}
)json");
    std::ostringstream out_invalid_url;
    EXPECT_THROW(
        (void)engine::generate_to_stream(
            cfg_invalid_url,
            engine::ExecutionOptions{.requested_threads = 1},
            out_invalid_url
        ),
        std::invalid_argument
    );

    const auto cfg_invalid_host = cfg_from_json(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"h","generator":"hostname","config":{"subdomains":["www"],"tlds":[]}}
  ]
}
)json");
    std::ostringstream out_invalid_host;
    EXPECT_THROW(
        (void)engine::generate_to_stream(
            cfg_invalid_host,
            engine::ExecutionOptions{.requested_threads = 1},
            out_invalid_host
        ),
        std::invalid_argument
    );

    const auto cfg3 = cfg_from_json(R"json(
{
  "rows": 1,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"d","generator":"file_directory","config":{"operating_systems":["Windows"]},"group":"file:missing"}
  ]
}
)json");
    std::ostringstream out3;
    EXPECT_THROW(
        (void)engine::generate_to_stream(cfg3, engine::ExecutionOptions{.requested_threads = 1}, out3),
        std::runtime_error
    );
}

TEST(LinkageConflictTest, FileExtensionLinkagePath) {
    const auto cfg = cfg_from_json(R"json(
{
  "rows": 3,
  "output": {
    "type": "file",
    "file": {
      "format": "json"
    }
  },
  "fields": [
    {"name":"p","generator":"file_path","config":{"operating_systems":["Windows"],"extensions":["txt"]},"group":"file:ext"},
    {"name":"e","generator":"file_extension","config":{"extensions":["txt"]},"group":"file:ext"}
  ]
}
)json");
    std::ostringstream out;
    EXPECT_NO_THROW(
        (void)engine::generate_to_stream(cfg, engine::ExecutionOptions{.requested_threads = 1}, out)
    );
    EXPECT_NE(out.str().find("\"e\""), std::string::npos);
}

}  // namespace
