# Data Generator

[![codecov](https://codecov.io/gh/ishizhongqi/data-generator/branch/develop/graph/badge.svg?token=TJZIICPRO1)](https://codecov.io/gh/ishizhongqi/data-generator)

Data Generator 是一个 C++ CLI，用 JSON 配置生成模拟数据，可写入 CSV/JSON/SQL 文件或通过 ODBC/SQLite 导入数据库。

**构建**

```sh
cmake -S . -B build
cmake --build build
```

生成的可执行文件名为 `data-generator`。若按上面的方式构建，可使用 `./build/data-generator` 运行，或将其加入 `PATH`。

**使用方法**

**CLI**

`data-generator <command> [options]`

命令概览：

- `help`：展示全部命令帮助。
- `init`：生成 JSON 配置模板。
- `preview`：生成单行预览。
- `run`：根据 JSON 配置生成完整数据集。
- `info`：列出或查看生成器说明。
- `drivers`：列出已安装的 ODBC 驱动。
- `check`：校验 JSON 配置。
- `schema`：输出或写入 JSON Schema。

全局选项：

- `-h, --help`：显示帮助。
- `-v, -V, --version`：显示版本号。

**命令：`init`**

参数：

- `<json>`：输出 JSON 文件路径。必填。
- `--template <file|database>`：模板类型。默认 `file`。
- `--format <csv|json|sql|Tab-Delimited|Custom>`：文件模板格式（仅 file 模板）。
- `--from-database <url>`：用于推断字段的数据库 URL（ODBC 或 SQLite，仅 database 模板）。
- `--table <name>`：目标表名（database 模板或 SQL 文件模板）。
- `-h, --help`：显示帮助。

**命令：`preview`**

参数：

- `<json>`：输入 JSON 配置文件。必填。
- `--field <name>`：预览字段名。可选。
- `-h, --help`：显示帮助。

**命令：`run`**

参数：

- `<json>`：输入 JSON 配置文件。必填。
- `--output <file>`：文件输出路径（file 输出）。
- `--rows <N>`：覆盖 JSON 中的 rows。整数 `>= 1`。
- `-h, --help`：显示帮助。

**命令：`info`**

参数：

- `<name>`：生成器名称。可选。
- `--json`：JSON 格式输出。
- `-h, --help`：显示帮助。

**命令：`drivers`**

参数：

- `--json`：JSON 格式输出。
- `-h, --help`：显示帮助。

**命令：`check`**

参数：

- `<json>`：输入 JSON 配置文件。必填。
- `-h, --help`：显示帮助。

**命令：`schema`**

参数：

- `<file>`：输出 Schema 文件路径。必填。
- `-h, --help`：显示帮助。

**JSON 配置**

根级键：

- `$schema`：string。JSON Schema 路径。
- `rows`：integer。`run` 时必填。`>= 1`。
- `output`：object。输出配置。
- `fields`：array。必填。字段数组。

`output` 对象：

- `type`：string。`file` 或 `database`。
- `file`：object。`type=file` 时必填。
  - `format`：string。`csv`、`json`、`sql`、`Tab-Delimited` 或 `Custom`。
  - `options`：object。格式相关选项。
    - CSV：`header`（布尔）、`line_ending`（`LF` 或 `CRLF`）。
    - JSON：`array`（布尔）、`include_null`（布尔）。
    - SQL：`table`（字符串）、`create_table`（布尔）。
    - Tab-Delimited：`header`（布尔）、`line_ending`（`LF` 或 `CRLF`）。
    - Custom：`delimiter`（字符串）、`quote`（字符串）、`header`（布尔）、`line_ending`（`LF` 或 `CRLF`）。
- `database`：object。`type=database` 时必填。
  - `url`：string。必填。
  - `table`：string。必填。
  - `insert_mode`：string。`auto`、`insert`、`bulk`、`load`。
  - `batch_size`：integer。`>= 1`。
  - `queue_size`：integer。`>= 1`。
  - `threads`：integer。`>= 1`。
  - `transaction_mode`：string。`per-batch`、`per-run`、`none`。
  - `error_policy`：string。`stop`、`continue`、`rollback-batch`、`rollback-all`。
  - `rate_limit_rows_per_sec`：integer。`>= 1`。

SQLite URL 格式：

- `sqlite:/absolute/path/to/file.db`
- `sqlite::memory:`

字段对象键：

- `name`：string。字段/列名。
- `generator`：string。见下方生成器列表。
- `config`：object。生成器配置，使用 `data-generator info <name>` 查看。
- `unique`：boolean。仅对支持唯一的生成器有效。
- `data_linkage`：string。格式 `module:Group1`。
- `default_value`：object。按比例使用固定默认值。
- `null_value`：object。按比例输出空值。

`default_value` 对象：

- `enabled`：boolean。是否启用默认值覆盖。
- `percent`：integer。`0` 到 `100`。
- `value`：string。覆盖时使用的固定值。

`null_value` 对象：

- `enabled`：boolean。是否启用空值覆盖。
- `percent`：integer。`0` 到 `100`。

若同时启用 `default_value` 与 `null_value`，两者 `percent` 之和必须 `<= 100`。

支持的生成器：

```
company_name, department, industry, ip_address, mac_address, file_path, file_directory, file_name,
file_extension, url, hostname, date, time, datetime, address_line1, address_line2, postcode,
full_address, city, region, integer, decimal, payment_method, card_type, card_number, card_date,
first_name, last_name, full_name, gender, title, marital_status, phone_number, email, job_title,
social_network_id, product_name, product_category, color, size, barcode, enum_item, text, uuid,
sequence, regular_expression
```

使用 `data-generator info <name>` 可查看每个生成器的配置字段与支持的值。

**示例**

数据库建表示例：

- `docs/schema_mysql.sql`
- `docs/schema_oracle.sql`
- `docs/schema_postgresql.sql`

JSON 配置示例：

- `docs/example_mysql_db.json`
- `docs/example_file.json`

示例命令：

```sh
# 校验配置
./build/data-generator check docs/example_mysql_db.json

# 预览单行（格式来自 JSON 配置）
./build/data-generator preview docs/example_file.json

# 生成 CSV 文件
./build/data-generator run docs/example_file.json --output ./out.csv

# 通过 ODBC 写入 MySQL
./build/data-generator run docs/example_mysql_db.json
```

**许可证**

MIT，详见 `LICENSE`。
