# Data Generator

[![codecov](https://codecov.io/gh/ishizhongqi/data-generator/branch/develop/graph/badge.svg?token=TJZIICPRO1)](https://codecov.io/gh/ishizhongqi/data-generator)

Data Generator 是一个 C++ CLI，用 JSON 配置生成模拟数据，可写入 CSV/JSON/SQL 文件或通过 ODBC/SQLite 导入数据库。

> 说明：本项目中的多数代码由 AI 辅助生成。

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
- `--from-database <connection>`：用于推断字段的数据库连接，格式为 `odbc://...` 或 `sqlite://...`（仅 database 模板）。
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
  - `connection`：string。必填。使用 `odbc://...` 或 `sqlite://...`。
  - `table`：string。必填。
  - `insert_mode`：string。`auto`、`insert`、`bulk`、`load`。
  - `batch_size`：integer。`>= 1`。
  - `queue_size`：integer。`>= 1`。
  - `threads`：integer。`>= 1`。
  - `transaction_mode`：string。`per-batch`、`per-run`、`none`。
  - `error_policy`：string。`stop`、`continue`、`rollback-batch`、`rollback-all`。
  - `rate_limit_rows_per_sec`：integer。`>= 1`。

`output.database` 详细说明：

- `insert_mode`：控制写入数据库的方式。
  - `auto`：当驱动支持 `load` 且 `rows >= 50000` 时选择 `load`；否则若 `batch_size > 1` 则选择 `bulk`；再否则选择 `insert`。
  - `insert`：每一行执行一条 `INSERT INTO ... VALUES (...)`。
  - `bulk`：按批写入。
    - MySQL/PostgreSQL/SQLite：`INSERT INTO ... VALUES (...), (...), ...`
    - Oracle：`INSERT ALL ... INTO ... VALUES (...) ... SELECT 1 FROM DUAL`
    - 如果当前数据库不支持多行 `VALUES`，后端会自动退回到逐行 `insert`。
  - `load`：先在 `<workspace>/tmp/` 下生成临时数据文件，再让数据库执行导入。
    - MySQL：`LOAD DATA LOCAL INFILE`
    - PostgreSQL：`COPY ... FROM`
    - SQLite：当前不支持
    - Oracle：当前后端未实现
  - 说明：
    - SQLite 只适合 `insert` 或 `bulk`；显式使用 `load` 时会在运行时报不支持。
    - MySQL 的 `load` 模式如果被驱动或服务端禁用了 `LOAD DATA LOCAL INFILE`，当前实现会回退到 `bulk`。
- `batch_size`：每批写入的行数。默认 `1000`。
  - 对 `bulk` 和 `load` 直接生效，也会在 `transaction_mode=per-batch` 时决定每个事务包含多少行。
  - 即使是 `insert`，程序也会先按批缓存行，只是批内每一行仍分别发送单条 `INSERT`。
- `queue_size`：生成线程与数据库写线程之间的内存队列容量。默认 `1024`。
  - 值更大时，生产和写入速度不一致时会更平滑。
  - 值更小时，内存占用更低，也会更早对生成端施加背压。
- `threads`：数据库写线程数量。默认 `2`。
  - SQLite 会被强制降为 `1`，因为当前后端只支持单写者。
  - `transaction_mode=per-run` 也会被强制降为 `1`，因为当前实现不会把同一个运行级事务跨多个写线程共享。
  - 这个 JSON 字段在内部配置中对应 `db_threads`。
- `transaction_mode`：控制事务边界。
  - `per-batch`：每个批次开启一个事务，批次成功后 `COMMIT`，失败时 `ROLLBACK` 当前批次。
  - `per-run`：整个运行只开启一个事务，结束时统一 `COMMIT` 或 `ROLLBACK`。
  - `none`：不显式开启事务，使用数据库/驱动的默认行为。
  - 分数据库行为：
    - SQLite：使用 `BEGIN TRANSACTION`
    - MySQL/PostgreSQL：使用 `START TRANSACTION`
    - Oracle：当前后端不会发送显式的 begin SQL，但需要时仍会执行 `COMMIT` 和 `ROLLBACK`
- `error_policy`：控制写入出错后的处理方式。
  - `stop`：立即停止，并报告首个错误。
  - `continue`：记录警告后继续处理后续行或批次。
  - `rollback-batch`：如果当前存在批次级事务，则回滚当前批次，然后继续。
  - `rollback-all`：停止后续处理；如果当前存在运行级事务，则回滚整个运行。
  - 说明：
    - `rollback-batch` 最适合搭配 `transaction_mode=per-batch`。
    - `rollback-all` 最适合搭配 `transaction_mode=per-run`。
    - 如果 `transaction_mode=none`，则没有显式事务可回滚，此时 rollback 类策略主要体现为“停止还是继续”。
- `rate_limit_rows_per_sec`：成功写入后的目标限速，单位为每秒行数。默认 `20000`。
  - 后端会在每个成功批次后适当休眠，使整体导入速率接近这个值。
  - 它限制的是数据库写入吞吐，不只是数据生成速度。

连接格式：

- ODBC：`odbc://DRIVER={MySQL ODBC 8.0 Driver};SERVER=127.0.0.1;PORT=3306;DATABASE=test;UID=root;PWD=123456;`
- SQLite 文件：`sqlite:///absolute/path/to/file.db`
- SQLite 内存库：`sqlite://:memory:`

对于 ODBC，`odbc://` 之后的内容会原样传给 `SQLDriverConnect`。

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
- `percentage`：integer。`0` 到 `100`。
- `value`：string。覆盖时使用的固定值。

`null_value` 对象：

- `enabled`：boolean。是否启用空值覆盖。
- `percentage`：integer。`0` 到 `100`。

若同时启用 `default_value` 与 `null_value`，两者 `percentage` 之和必须 `<= 100`。

支持的生成器(来自库 `faker` 的接口)：

```
company_name, department, industry, ip_address, mac_address, file_path, file_directory, file_name,
file_extension, url, hostname, date, time, datetime, address_line1, address_line2, postcode,
full_address, city, region, integer, decimal(decimal_string), payment_method, card_type, card_number, card_date,
first_name, last_name, full_name, gender, title, marital_status, phone_number, email, job_title,
social_network_id, product_name, product_category, color, size, barcode, enum_item, text, uuid,
boolean, sequence, regular_expression
```

使用 `data-generator info <name>` 可查看每个生成器的配置字段与支持的值。

`boolean` 生成器：

- 所属模块：`utility`
- 配置项：
  - `true_percentage`：number。必填。范围 `0` 到 `100`。示例默认值为 `50`
- 输出行为：
  - JSON 文件：输出原生 JSON 布尔值 `true` / `false`
  - SQL 文件：输出 SQL 字面量 `TRUE` / `FALSE`
  - CSV / Tab-Delimited / Custom 文件：输出文本 `true` / `false`
  - 数据库输出：
    - PostgreSQL `BOOLEAN`：写入 `TRUE` / `FALSE`
    - MySQL `TINYINT(1)`、Oracle `NUMBER(1)`、SQLite `INTEGER`：写入 `1` / `0`
    - 其他字符串类列：如果 schema 校验允许，则按文本 `true` / `false` 写入

字段示例：

```json
{
  "name": "is_active",
  "generator": "boolean",
  "config": {
    "true_percentage": 80
  }
}
```

**示例**

数据库建表示例：

- `docs/schema_mysql.sql`
- `docs/schema_oracle.sql`
- `docs/schema_postgresql.sql`
- `docs/schema_sqlite.sql`

JSON 配置示例：

- `docs/example_mysql_db.json`
- `docs/example_postgresql_db.json`
- `docs/example_oracle_db.json`
- `docs/example_sqlite_db.json`
- `docs/example_file.json`

示例命令：

```sh
# 校验配置
data-generator check example_mysql_db.json

# 预览单行（格式来自 JSON 配置）
data-generator preview example_file.json

# 生成 CSV 文件
data-generator run example_file.json --output out.csv

# 通过 ODBC 写入 MySQL
data-generator run example_mysql_db.json
```

**许可证**

MIT，详见 `LICENSE`。
