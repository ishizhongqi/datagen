// Copyright (c) 2025 Shizhongqi
// Licensed under the MIT License.
// See the LICENSE file in the project root for more information.

/// @file main.cpp

// You can include the full header <faker/faker.h>, or include individual module headers like <faker/number.h>.
#include <faker/faker.h>

#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

#include "generator/computer_generators.h"
#include "generator/generator_registry.h"

namespace data_generator {

using Json = nlohmann::json;
using generator::GeneratorRegistry;
using generator::IGenerator;

int run() {
    GeneratorRegistry registry;
    register_computer_generators(registry);

    Json root = {
        {"rows", 10},
        {"columns",
         Json::array(
             {{{"name", "ip"},
               {"generator", "ip_address"},
               {"config", {{"ip_address_type", "IPv4"}}},
               {"unique", true}},
              {{"name", "mac"}, {"generator", "mac_address"}, {"config", Json::object()}, {"unique", true}},
              {{"name", "file_path"},
               {"generator", "file_path"},
               {"config", {{"operating_systems", {"macOS"}}, {"extensions", {"jpg", "png", "txt"}}}},
               {"data_linkage", true}},
              {{"name", "file_directory"},
               {"generator", "file_directory"},
               {"config", {{"operating_systems", {"macOS"}}}},
               {"data_linkage", true}},
              {{"name", "file_extension"},
               {"generator", "file_extension"},
               {"config", {{"extensions", {"jpg", "png", "txt"}}}},
               {"data_linkage", true}},
              {{"name", "file_name"},
               {"generator", "file_name"},
               {"config", {{"extensions", {"jpg", "png", "txt"}}}},
               {"data_linkage", true}}}
         )}
    };

    std::vector<std::pair<std::string, std::unique_ptr<IGenerator>>> columns;

    for (const auto& col : root["columns"]) {
        const std::string name      = col.at("name").get<std::string>();
        const std::string generator = col.at("generator").get<std::string>();

        auto gen = registry.create(generator, col);
        columns.emplace_back(name, std::move(gen));
    }

    for (int row = 0; row < root["rows"].get<int>(); ++row) {
        std::cout << "Row " << row << "\n";
        for (auto& [name, gen] : columns) { std::cout << "  " << name << " = " << gen->generate() << "\n"; }
        for (auto& [_, gen] : columns) { gen->next(); }
    }

    // // You need to use a Bilingual object to store the return values from functions that provide bilingual output.
    //  const faker::Bilingual first_name_bilingual   = faker::person::first_name(faker::Languages::SimplifiedChinese);
    //  const std::string      first_name_original    = first_name_bilingual.original();
    //  const std::string      first_name_translation = first_name_bilingual.translation();
    //  std::cout << "First name: " << first_name_original << " (" << first_name_translation << ")" << std::endl;
    //
    //  // Most function parameters have default values, so you can just call the functions without providing any
    //  arguments. const std::string industry = faker::business::industry(); std::cout << "Industry: " << industry <<
    //  std::endl;
    //
    //  // Some functions allow bitwise OR (|) in their parameters,
    //  // which you can use when you want to generate multiple enum members at once.
    //  const std::string date =
    //      faker::datetime::date("2023-01-01", "2023-12-31", faker::DaysOfWeek::Monday | faker::DaysOfWeek::Thursday);
    //  std::cout << "Date: " << date << std::endl;
    //
    //  // You can use a class to create an entity
    //  // where the generated data has stronger correlations between its fields.
    //  // Of course, you can choose to call it with or without parameters.
    //  const faker::person::Person person;
    //  // At this point, the fake data has been fully generated, and you just need to call the getters to get data.
    //  std::cout << "Person.First name : " << person.first_name().original() << std::endl;
    //  std::cout << "Person.Full name  : " << person.full_name().original() << std::endl;
    //  std::cout << "Person.Gender     : " << person.gender() << std::endl;
    //  std::cout << "Person.Email      : " << person.email() << std::endl;
    //
    //  // File
    //  std::cout << "File path       : " << faker::computer::file_path() << std::endl;
    //  std::cout << "File directory  : " << faker::computer::file_directory() << std::endl;
    //  std::cout << "File name       : " << faker::computer::file_name() << std::endl;
    //  std::cout << "File extension  : " << faker::computer::file_extension() << std::endl;
    //
    //  faker::computer::File file(
    //      faker::OperatingSystems::Windows,
    //      std::to_array<std::string_view>({"jpg", "png", "txt", "pdf"})
    //  );
    //  std::cout << "File.Path       : " << file.path() << std::endl;
    //  std::cout << "File.Directory  : " << file.directory() << std::endl;
    //  std::cout << "File.Name       : " << file.name() << std::endl;
    //  std::cout << "File.Extension  : " << file.extension() << std::endl;
    //  file.reroll();
    //  std::cout << "File.Path       : " << file.path() << std::endl;
    //  std::cout << "File.Directory  : " << file.directory() << std::endl;
    //  std::cout << "File.Name       : " << file.name() << std::endl;
    //  std::cout << "File.Extension  : " << file.extension() << std::endl;
    //
    //  // For the other functions and classes, see the source code comments or the documentation.
    //
    //  // 测试生成器 - 根据JSON配置文件生成数据
    //  // -----------------------------------
    //  // 1. 初始化 registry（程序启动一次）
    //  // -----------------------------------
    //  GeneratorRegistry registry;
    //  register_computer_generators(registry);
    //
    //  // -----------------------------------
    //  // 2. 根据JSON配置文件创建生成器
    //  // -----------------------------------
    //  // 模拟从input_example.json解析的配置
    //  Json ip_config = {{"ip_address_type", "IPv4"}, {"unique", true}};
    //
    //  // file_path配置（包含data_linkage）
    //  Json file_path_config = {
    //      {"operating_systems", {"Windows"}},
    //      {"extensions", {"jpg", "png", "txt", "pdf"}},
    //      {"data_linkage", true}
    //  };
    //
    //  // file_directory配置（包含data_linkage）
    //  Json file_directory_config = {{"operating_systems", {"Windows"}}, {"data_linkage", true}};
    //
    //  // file_name配置（包含data_linkage）
    //  Json file_name_config = {{"extensions", {"jpg", "png", "txt", "pdf"}}, {"data_linkage", true}};
    //
    //  // file_extension配置（包含data_linkage）
    //  Json file_extension_config = {{"extensions", {"jpg", "png", "txt", "pdf"}}, {"data_linkage", true}};
    //
    //  // -----------------------------------
    //  // 3. 创建列生成器（一列一个）
    //  // -----------------------------------
    //  std::unique_ptr<IGenerator> ip_generator             = registry.create("ip_address", ip_config);
    //  std::unique_ptr<IGenerator> file_path_generator      = registry.create("file_path", file_path_config);
    //  std::unique_ptr<IGenerator> file_directory_generator = registry.create("file_directory", file_directory_config);
    //  std::unique_ptr<IGenerator> file_name_generator      = registry.create("file_name", file_name_config);
    //  std::unique_ptr<IGenerator> file_extension_generator = registry.create("file_extension", file_extension_config);
    //
    //  // -----------------------------------
    //  // 4. 模拟一张表的列
    //  // -----------------------------------
    //  std::vector<std::pair<std::string, IGenerator*>> columns = {
    //      {"ip_address", ip_generator.get()},
    //      {"file_path", file_path_generator.get()},
    //      {"file_directory", file_directory_generator.get()},
    //      {"file_name", file_name_generator.get()},
    //      {"file_extension", file_extension_generator.get()}
    //  };
    //
    //  // -----------------------------------
    //  // 5. 生成数据（核心逻辑）
    //  // -----------------------------------
    //  std::cout << "\n=== 数据生成示例（带数据关联） ===\n";
    //  constexpr int kRowCount = 3;
    //  for (int row = 0; row < kRowCount; ++row) {
    //      std::cout << "\nRow " << row + 1 << ":\n";
    //      std::cout << "----------------------------------------\n";
    //      for (const auto& [column_name, generator] : columns) {
    //          std::string value = generator->generate();
    //          std::cout << std::setw(15) << std::left << column_name << " = " << value << "\n";
    //      }
    //      // 行结束：所有 generator 进入下一行
    //      for (const auto& generator : columns) { generator.second->next(); }
    //  }
    //
    //  // 测试无数据关联的情况
    //  std::cout << "\n=== 数据生成示例（无数据关联） ===\n";
    //  Json file_path_no_linkage = {
    //      {"operating_systems", {"Windows"}},
    //      {"extensions", {"jpg", "png"}},
    //      {"data_linkage", false}
    //  };
    //
    //  Json file_directory_no_linkage = {{"operating_systems", {"Windows"}}, {"data_linkage", false}};
    //
    //  auto file_path_no_linkage_gen      = registry.create("file_path", file_path_no_linkage);
    //  auto file_directory_no_linkage_gen = registry.create("file_directory", file_directory_no_linkage);
    //
    //  std::cout << "\n无数据关联示例（文件路径和目录不相关）：\n";
    //  for (int i = 0; i < 2; ++i) {
    //      std::cout << "文件路径: " << file_path_no_linkage_gen->generate() << std::endl;
    //      std::cout << "文件目录: " << file_directory_no_linkage_gen->generate() << std::endl;
    //      std::cout << "---\n";
    //      file_path_no_linkage_gen->next();
    //      file_directory_no_linkage_gen->next();
    //  }

    return 0;
}

}  // namespace data_generator

int main() {
    data_generator::run();
    return 0;
}
