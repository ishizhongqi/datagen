# Datagen

[![codecov](https://codecov.io/gh/ishizhongqi/datagen/branch/develop/graph/badge.svg?token=TJZIICPRO1)](https://codecov.io/gh/ishizhongqi/datagen)

Datagen is a C++ CLI that generates synthetic datasets from a JSON configuration. It can write CSV/JSON/SQL files or insert rows into databases via ODBC or SQLite.

> Note: Most of the code in this project was generated with AI assistance.

**Usage**

**CLI**

`datagen <command> [options]`

Command summary:

- `help`: Show help for all commands.
- `init`: Generate a JSON configuration template.
- `preview`: Generate preview rows.
- `run`: Generate full dataset from JSON config.
- `info`: List generators or show generator details.
- `drivers`: List installed ODBC drivers.
- `check`: Validate a JSON config.
- `schema`: Print or write the JSON Schema.

Global options:

- `-h, --help`: Show help.
- `-v, -V, --version`: Show version.

**Command: `init`**

Options:

- `<json>`: Output JSON file path. Required.
- `--template <file|database>`: Template type. Default `file`.
- `--format <csv|json|sql|Tab-Delimited|Custom>`: File template format (file template only).
- `--from-database <connection>`: Database connection in `odbc://...` or `sqlite://...` format for schema inference (database template only).
- `--table <name>`: Target table for database templates or SQL file templates.
- `-h, --help`: Show help.

**Command: `preview`**

Options:

- `<json>`: Input JSON config file. Required.
- `--rows <N>`: Preview row count. Integer `>= 1`. Default `1`.
- `--field <name>`: Field name to preview. Optional.
- `-h, --help`: Show help.

**Command: `run`**

Options:

- `<json>`: Input JSON config file. Required.
- `--output <file>`: Output file path for file output.
- `--rows <N>`: Override rows in JSON. Integer `>= 1`.
- `-h, --help`: Show help.

**Command: `info`**

Options:

- `<name>`: Generator name. Optional.
- `--json`: Output JSON instead of text.
- `-h, --help`: Show help.

**Command: `drivers`**

Options:

- `--json`: Output JSON instead of plain text.
- `-h, --help`: Show help.

**Command: `check`**

Options:

- `<json>`: Input JSON config file. Required.
- `-h, --help`: Show help.

**Command: `schema`**

Options:

- `<file>`: Output schema file path. Required.
- `-h, --help`: Show help.

**JSON Configuration**

Root keys:

- `$schema`: string. JSON Schema path.
- `rows`: integer. Required for `run`. `>= 1`.
- `output`: object. Output configuration.
- `fields`: array. Required. Array of field objects.

`output` object:

- `type`: string. `file` or `database`.
- `file`: object. Required when `type=file`.
  - `format`: string. `csv`, `json`, `sql`, `Tab-Delimited`, or `Custom`.
  - `options`: object. Format-specific options.
    - CSV: `header` (boolean), `line_ending` (`LF` or `CRLF`).
    - JSON: `array` (boolean), `include_null` (boolean).
    - SQL: `table` (string), `create_table` (boolean).
    - Tab-Delimited: `header` (boolean), `line_ending` (`LF` or `CRLF`).
    - Custom: `delimiter` (string), `quote` (string), `header` (boolean), `line_ending` (`LF` or `CRLF`).
- `database`: object. Required when `type=database`.
  - `connection`: string. Required. Use `odbc://...` or `sqlite://...`.
  - `table`: string. Required.
  - `mode`: string. `fast`, `balanced`, or `safe`. Default `balanced`.
  - `write_mode`: string. `append`, `truncate`, or `upsert`. Default `append`.
  - `transaction_mode`: string. `per-batch`, `per-run`, `none`.
  - `error_policy`: string. `stop`, `continue`, or `rollback`. Default `stop`.
  - `advanced`: object. Optional. Overrides the defaults implied by `mode`.
    - `insert_mode`: string. `insert` or `load`.
    - `batch_size`: integer. `>= 1`.
    - `queue_size`: integer. `>= 1`.
    - `threads`: integer. `>= 1`.
    - `rate_limit_rows_per_sec`: integer. `>= 0`. `0` means unlimited.

`output.database` details:

- `mode`: quick presets for the advanced parameters.
  - `fast`: `load`, `5000`, `5120`, `8`, `0`
  - `balanced`: `insert`, `2000`, `2048`, `2`, `20000`
  - `safe`: `insert`, `1000`, `1024`, `1`, `5000`
  - The values above are, in order: `insert_mode`, `batch_size`, `queue_size`, `threads`, `rate_limit_rows_per_sec`.
- `advanced`: explicit overrides for the defaults implied by `mode`.
  - If `advanced` is omitted, the current `mode` is used as-is.
  - If one key is present under `advanced`, only that key is overridden.
- `write_mode`: controls what happens before a row is written.
  - `append`: append new rows.
  - `truncate`: clear the target table before generation.
  - `upsert`: update existing rows when a generated primary key or unique key already exists, otherwise insert.
- `insert_mode`: controls how rows reach the database.
  - `insert`: batched SQL inserts. This is the old bulk-insert behavior.
  - `load`: write a temporary data file under `<workspace>/tmp/`, then ask the database to import it.
  - If the database or current `write_mode` does not support `load`, the backend falls back to `insert` and logs the reason.
- `batch_size`: rows per write batch.
  - This affects SQL batch size, and also the transaction chunk when `transaction_mode=per-batch`.
- `queue_size`: capacity of the in-memory queue between the generator side and the database writer side.
  - The queue stores generated rows waiting to be imported.
  - If the queue becomes full, generation blocks until writers catch up. So this is a throughput-vs-memory knob, not a correctness knob.
  - As a practical rule, start with the preset default from `mode`. Increase it when generation is much faster than imports and you want smoother throughput. Decrease it when memory usage matters more than peak throughput.
  - In most cases, keeping `queue_size` around `1x` to `4x` of `batch_size` is a reasonable starting point.
- `threads`: number of database writer threads, not the number of generator threads.
  - It does take effect: the backend starts that many writer workers when the database and transaction mode allow it.
  - SQLite is forced to `1`, because this backend only supports a single SQLite writer.
  - `transaction_mode=per-run` is also forced to `1`, because one run-wide transaction is not shared across multiple writers here.
  - So if you set `threads=8`, the log may still show `Threads: 1 (...)` when the runtime has to downgrade it.
- `transaction_mode`: controls transaction boundaries.
  - `per-batch`: begin one transaction for each batch, then `COMMIT` or `ROLLBACK` that batch.
  - `per-run`: begin one transaction for the whole run, then `COMMIT` or `ROLLBACK` once at the end.
  - `none`: do not explicitly start a transaction; each statement uses the database/driver default behavior.
  - Per database:
    - SQLite: begins with `BEGIN TRANSACTION`
    - MySQL/PostgreSQL: begins with `START TRANSACTION`
    - Oracle: no explicit begin SQL is sent by the current backend; `COMMIT` and `ROLLBACK` are still used when needed
- `error_policy`: controls what happens after a batch or statement error.
  - `stop`: stop immediately and report the first error.
  - `continue`: log a warning and keep processing later rows/batches.
  - `rollback`: only available with `insert_mode=insert`.
  - With `transaction_mode=per-batch`, `rollback` rolls back the current batch and continues.
  - With `transaction_mode=per-run`, `rollback` rolls back the full run and stops.
  - Notes:
    - If `insert_mode` is not `insert`, rollback is downgraded to `stop`.
    - With `transaction_mode=none`, there is no explicit transaction to roll back.
- `rate_limit_rows_per_sec`: maximum import speed target after successful writes.
  - The backend sleeps after each successful batch so the effective import rate stays near this value.
  - This limits database write throughput, not just row generation speed.

Connection format:

- ODBC: `odbc://DRIVER={MySQL ODBC 8.0 Driver};SERVER=127.0.0.1;PORT=3306;DATABASE=test;UID=root;PWD=123456;`
- SQLite file: `sqlite:///absolute/path/to/file.db`
- SQLite memory: `sqlite://:memory:`

For ODBC, everything after `odbc://` is passed directly to `SQLDriverConnect`.

Field object keys:

- `name`: string. Column/field name.
- `generator`: string. See supported generators below.
- `config`: object. Generator-specific config. Use `datagen info <name>` for details.
- `unique`: boolean. Only for generators that support unique output.
- `group`: string. Any non-empty string.
  - Group linkage is still isolated by generator module internally, so the same group value in different modules does not mix records together.
- `default_value`: object. Overrides a percentage of rows with a fixed value.
- `null_value`: object. Overrides a percentage of rows with nulls.

`default_value` object:

- `enabled`: boolean. Whether to enable default override.
- `percentage`: integer. `0` to `100`.
- `value`: string. Default value when override triggers.

`null_value` object:

- `enabled`: boolean. Whether to enable null override.
- `percentage`: integer. `0` to `100`.

If both `default_value` and `null_value` are enabled, their `percentage` values must sum to `<= 100`.

Supported generators (from library `faker`):

```
company_name, department, industry, ip_address, mac_address, file_path, file_directory, file_name,
file_extension, url, hostname, date, time, datetime, address_line1, address_line2, postcode,
full_address, city, region, integer, decimal(decimal_string), payment_method, card_type, card_number, card_date,
first_name, last_name, full_name, gender, title, marital_status, phone_number, email, job_title,
social_network_id, product_name, product_category, color, size, barcode, enum_item, text, uuid,
boolean, sequence, regular_expression
```

Use `datagen info <name>` to see required config keys and supported values for each generator.

`boolean` generator:

- Module: `utility`
- Config:
  - `true_percentage`: number. Required. Range `0` to `100`. Default example value is `50`.
- Output behavior:
  - JSON file output: writes native JSON booleans `true` / `false`
  - SQL file output: writes SQL literals `TRUE` / `FALSE`
  - CSV / Tab-Delimited / Custom file output: writes text `true` / `false`
  - Database output:
    - PostgreSQL `BOOLEAN`: writes `TRUE` / `FALSE`
    - MySQL `TINYINT(1)`, Oracle `NUMBER(1)`, SQLite `INTEGER`: writes `1` / `0`
    - Other string-like columns: stores `true` / `false` as text if schema validation allows it

Example field:

```json
{
  "name": "is_active",
  "generator": "boolean",
  "config": {
    "true_percentage": 80
  }
}
```

**Examples**

Database schemas:

- `docs/schema_mysql.sql`
- `docs/schema_oracle.sql`
- `docs/schema_postgresql.sql`
- `docs/schema_sqlite.sql`

JSON config examples:

- `docs/example_mysql_db.json`
- `docs/example_postgresql_db.json`
- `docs/example_oracle_db.json`
- `docs/example_sqlite_db.json`
- `docs/example_file.json`

Example commands:

```sh
# Validate config
datagen check example_mysql_db.json

# Preview five rows (file output keeps the configured file format, database output uses CSV)
datagen preview example_file.json --rows 5

# Generate CSV to file
datagen run example_file.json --output out.csv

# Generate data into MySQL (ODBC)
datagen run example_mysql_db.json
```

**License**

MIT. See `LICENSE`.
