# Datagen

[![codecov](https://codecov.io/gh/ishizhongqi/datagen/branch/develop/graph/badge.svg?token=TJZIICPRO1)](https://codecov.io/gh/ishizhongqi/datagen)

Datagen 是一个 C++ CLI，用 JSON 配置生成模拟数据，可写入 CSV/JSON/SQL 文件或通过 ODBC/SQLite 导入数据库。

> 说明：本项目中的多数代码由 AI 辅助生成。

**使用方法**

**CLI**

`datagen <command> [options]`

命令概览：

- `help`：展示全部命令帮助。
- `init`：生成 JSON 配置模板。
- `preview`：生成预览行。
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
- `--rows <N>`：预览行数。整数 `>= 1`。默认 `1`。
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
  - `mode`：string。`fast`、`balanced` 或 `safe`。默认 `balanced`。
  - `write_mode`：string。`append`、`truncate` 或 `upsert`。默认 `append`。
  - `transaction_mode`：string。`per-batch`、`per-run`、`none`。
  - `error_policy`：string。`stop`、`continue` 或 `rollback`。默认 `stop`。
  - `advanced`：object。可选。用于覆盖 `mode` 隐含的默认值。
    - `insert_mode`：string。`insert` 或 `load`。
    - `batch_size`：integer。`>= 1`。
    - `queue_size`：integer。`>= 1`。
    - `threads`：integer。`>= 1`。
    - `rate_limit_rows_per_sec`：integer。`>= 0`。`0` 表示不限速。

`output.database` 详细说明：

- `mode`：高级参数的快捷预设。
  - `fast`：`load`、`5000`、`5120`、`8`、`0`
  - `balanced`：`insert`、`2000`、`2048`、`2`、`20000`
  - `safe`：`insert`、`1000`、`1024`、`1`、`5000`
  - 上面这组值按顺序分别对应：`insert_mode`、`batch_size`、`queue_size`、`threads`、`rate_limit_rows_per_sec`。
- `advanced`：用于显式覆盖 `mode` 提供的默认值。
  - 不写 `advanced` 时，直接使用当前 `mode` 的默认组合。
  - 只写其中某几个键时，也只覆盖这些键。
- `write_mode`：控制写入前对目标表的处理方式。
  - `append`：直接追加数据。
  - `truncate`：生成前先清空目标表。
  - `upsert`：如果生成出的主键或唯一键已存在则更新，不存在则插入。
- `insert_mode`：控制数据如何进入数据库。
  - `insert`：批量 SQL 插入。这就是旧版 `bulk` 的行为。
  - `load`：先在 `<workspace>/tmp/` 下生成临时文件，再让数据库执行导入。
  - 如果当前数据库或当前 `write_mode` 不支持 `load`，后端会自动回退到 `insert`，并在日志里说明原因。
- `batch_size`：每次写入数据库的批大小。
  - 它会影响 SQL 批量写入大小，也会在 `transaction_mode=per-batch` 时决定每个事务包含多少行。
- `queue_size`：生成端和数据库写入端之间的内存队列容量。
  - 队列里存放的是“已经生成、等待导入”的行。
  - 队列满了以后，生成端会阻塞等待写入端追上。因此它是“吞吐量和内存占用”的调参项，不是“正确性开关”。
  - 作为使用者，建议先用 `mode` 的默认值。如果生成明显快于导入，希望整体更平滑，可以增大它；如果更在意内存占用，可以减小它。
  - 一般把 `queue_size` 放在 `batch_size` 的 `1x` 到 `4x` 左右，是一个比较稳妥的起点。
- `threads`：数据库写线程数，不是生成器线程数。
  - 它是会生效的，前提是当前数据库类型和事务模式允许多写线程。
  - SQLite 会被强制降为 `1`，因为当前后端只支持单写者。
  - `transaction_mode=per-run` 也会被强制降为 `1`，因为当前实现不会把一个运行级事务分给多个写线程共享。
  - 所以即使你配置了 `threads=8`，日志里也可能看到 `Threads: 1 (...)`，这表示运行时为了安全自动降级了。
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
  - `rollback`：只在 `insert_mode=insert` 时有意义。
  - 当 `transaction_mode=per-batch` 时，`rollback` 会回滚当前批次并继续。
  - 当 `transaction_mode=per-run` 时，`rollback` 会回滚整个运行并停止。
  - 说明：
    - 如果 `insert_mode` 不是 `insert`，`rollback` 会自动降级为 `stop`。
    - 如果 `transaction_mode=none`，则没有显式事务可回滚。
- `rate_limit_rows_per_sec`：成功写入后的目标限速，单位为每秒行数。
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
- `config`：object。生成器配置，使用 `datagen info <name>` 查看。
- `unique`：boolean。仅对支持唯一的生成器有效。
- `group`：string。任意非空字符串都可以。
  - 组关联在内部仍然按生成器模块隔离，所以不同模块即使写了同一个 group 值，也不会串组。
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

使用 `datagen info <name>` 可查看每个生成器的配置字段与支持的值。

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
datagen check example_mysql_db.json

# 预览 5 行（file 输出保持配置中的文件格式，database 输出使用 CSV）
datagen preview example_file.json --rows 5

# 生成 CSV 文件
datagen run example_file.json --output out.csv

# 通过 ODBC 写入 MySQL
datagen run example_mysql_db.json
```

**许可证**

MIT，详见 `LICENSE`。
