# Data Generator

[![codecov](https://codecov.io/gh/ishizhongqi/data-generator/branch/develop/graph/badge.svg?token=TJZIICPRO1)](https://codecov.io/gh/ishizhongqi/data-generator)

Data Generator 是一个 C++ CLI，用 JSON 配置生成模拟数据，可写入 CSV/JSON/SQL 文件或通过 ODBC 导入数据库。

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
- `generate`：根据 JSON 配置生成完整数据集。
- `describe`：查看生成器与配置说明。
- `list`：列出可用生成器。
- `drivers`：列出已安装的 ODBC 驱动。
- `validate`：校验 JSON 配置，可选只校验数据库连接。
- `schema`：输出或写入 JSON Schema。

**命令：`init`**

参数：

- `--generator <name>`：只生成一个字段的模板。支持值：下方生成器列表中的名称。
- `--output <file>`：输出 JSON 文件路径。默认 `config_YYYYmmddHHMMSS.json`。
- `--rows <N>`：生成行数。整数 `>= 1`。
- `--destination <file|database>`：输出目标。`file` 或 `database`，默认 `file`。
- `--file-format <csv|json|sql>`：文件输出格式。仅当 `destination=file` 时有效。
- `--database-url <url>`：数据库 URL 或 ODBC 连接串。`destination=database` 时使用。
- `--table <name>`：目标表名。`destination=database` 或 `file-format=sql` 时使用。
- `-h, --help`：显示帮助。

**命令：`preview`**

参数：

- `--input <json>`：输入 JSON 配置文件。必填。
- `--field <name|all>`：预览字段名。默认 `all`。
- `--file-format <csv|json|sql>`：预览输出格式。可选覆盖。
- `-h, --help`：显示帮助。

**命令：`generate`**

参数：

- `--input <json>`：输入 JSON 配置文件。必填。
- `--file-output <file>`：文件输出路径（目标为 file）。未指定时输出为 `dgresult_YYYYmmddHHMMSS.<format>`。
- `--output <file>`：`--file-output` 的别名。
- `--rows <N>`：覆盖 JSON 中的 rows。整数 `>= 1`。
- `--file-format <csv|json|sql>`：覆盖文件格式。`destination=database` 时忽略。
- `--destination <file|database>`：覆盖输出目标。`file` 或 `database`。
- `--database-url <url>`：数据库 URL 或 ODBC 连接串。数据库输出且 JSON 中未提供时必填。
- `--table <name>`：目标表名。必须非空。
- `--threads <N>`：生成线程数。整数 `>= 1`，默认 `1`。
- `-h, --help`：显示帮助。

**命令：`describe`**

参数：

- `--generator <name>`：要描述的生成器名称。必填。
- `--json`：JSON 格式输出。
- `-h, --help`：显示帮助。

**命令：`list`**

参数：

- `-h, --help`：显示帮助。

**命令：`drivers`**

参数：

- `--json`：JSON 格式输出。
- `-h, --help`：显示帮助。

**命令：`validate`**

参数：

- `--input <json>`：输入 JSON 配置文件。必填。
- `--db-only`：仅校验数据库连接。需要 `database_url`。
- `-h, --help`：显示帮助。

**命令：`schema`**

参数：

- `--output <file>`：写入文件而不是输出到 stdout。
- `-h, --help`：显示帮助。

**JSON 配置**

根级键：

- `$schema`：string。JSON Schema 路径。
- `rows`：integer。`generate` 时必填。`>= 1`。
- `destination`：string。`file` 或 `database`，默认 `file`。
- `file_format`：string。`destination=file` 时必填。`csv`、`json` 或 `sql`。`destination=database` 时忽略。
- `null_value_string`：string 或 null。`null` 表示空值输出为空，字符串表示空值输出为该字面量。
- `table`：string。`sql` 或数据库输出时必填。
- `database_url`：string。`destination=database` 时必填。
- `database_insert_mode`：string。`auto`、`insert`、`bulk`、`load`。
- `database_batch_size`：integer。`>= 1`。
- `database_queue_size`：integer。`>= 1`。
- `database_threads`：integer。`>= 1`。
- `database_transaction_mode`：string。`per-batch`、`per-run`、`none`。
- `database_error_policy`：string。`stop`、`continue`、`rollback-batch`、`rollback-all`。
- `database_rate_limit_rows_per_sec`：integer。`>= 1`。
- `fields`：array。必填。字段数组。

字段对象键：

- `name`：string。字段/列名。
- `generator`：string。见下方生成器列表。
- `config`：object。生成器配置，使用 `data-generator describe --generator <name>` 查看。
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

使用 `data-generator describe --generator <name>` 可查看每个生成器的配置字段与支持的值。

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
./build/data-generator validate --input docs/example_mysql_db.json

# 预览单行 JSON
./build/data-generator preview --input docs/example_file.json --file-format json

# 生成 CSV 文件
./build/data-generator generate --input docs/example_file.json --file-output ./out.csv --file-format csv

# 通过 ODBC 写入 MySQL
./build/data-generator generate --input docs/example_mysql_db.json --destination database \
  --database-url "odbc:mysql:DRIVER={MySQL ODBC 8.0 Unicode Driver};SERVER=127.0.0.1;PORT=3306;DATABASE=example_db;UID=user;PWD=password;" \
  --table customer_profile
```

**许可证**

MIT，详见 `LICENSE`。
