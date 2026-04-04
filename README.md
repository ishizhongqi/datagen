# Data Generator

[![codecov](https://codecov.io/gh/ishizhongqi/data-generator/branch/develop/graph/badge.svg?token=TJZIICPRO1)](https://codecov.io/gh/ishizhongqi/data-generator)

Data Generator is a C++ CLI that generates synthetic datasets from a JSON configuration. It can write CSV/JSON/SQL files or insert rows into databases via ODBC or SQLite.

> Note: Most of the code in this project was generated with AI assistance.

**Usage**

**CLI**

`data-generator <command> [options]`

Command summary:

- `help`: Show help for all commands.
- `init`: Generate a JSON configuration template.
- `preview`: Generate a single-row preview.
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
  - `insert_mode`: string. `auto`, `insert`, `bulk`, `load`.
  - `batch_size`: integer. `>= 1`.
  - `queue_size`: integer. `>= 1`.
  - `threads`: integer. `>= 1`.
  - `transaction_mode`: string. `per-batch`, `per-run`, `none`.
  - `error_policy`: string. `stop`, `continue`, `rollback-batch`, `rollback-all`.
  - `rate_limit_rows_per_sec`: integer. `>= 1`.

`output.database` details:

- `insert_mode`: controls how rows are written.
  - `auto`: chooses `load` when the driver supports load mode and `rows >= 50000`; otherwise chooses `bulk` when `batch_size > 1`; otherwise chooses `insert`.
  - `insert`: executes one `INSERT INTO ... VALUES (...)` per row.
  - `bulk`: writes one batch at a time.
    - MySQL/PostgreSQL/SQLite: `INSERT INTO ... VALUES (...), (...), ...`
    - Oracle: `INSERT ALL ... INTO ... VALUES (...) ... SELECT 1 FROM DUAL`
    - If the current database does not support multi-row `VALUES`, the backend falls back to row-by-row `insert`.
  - `load`: writes a temporary data file under `<workspace>/tmp/`, then asks the database to import it.
    - MySQL: `LOAD DATA LOCAL INFILE`
    - PostgreSQL: `COPY ... FROM`
    - SQLite: not supported
    - Oracle: not supported in the current backend
  - Notes:
    - SQLite uses `insert` or `bulk`; `load` is rejected at runtime.
    - MySQL load mode can fall back to `bulk` if `LOAD DATA LOCAL INFILE` is disabled by the driver or server.
- `batch_size`: rows per write batch. Default `1000`.
  - Used by `bulk` and `load`, and also defines the transaction chunk when `transaction_mode=per-batch`.
  - With `insert`, rows are still grouped in memory by batch, but each row in the batch is sent as its own `INSERT`.
- `queue_size`: capacity of the in-memory queue between the generator thread and database writer thread(s). Default `1024`.
  - Larger values can smooth producer/consumer imbalance.
  - Smaller values reduce memory usage and apply backpressure earlier.
- `threads`: number of database writer threads. Default `2`.
  - SQLite is forced to `1` because it only supports a single writer in this backend.
  - `transaction_mode=per-run` is also forced to `1`, because one transaction cannot be safely shared across multiple writer threads here.
  - In the internal config this field maps to `db_threads`.
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
  - `rollback-batch`: roll back only the current batch when a batch transaction exists, then continue.
  - `rollback-all`: stop further processing and roll back the whole run when a per-run transaction exists.
  - Notes:
    - `rollback-batch` is most meaningful with `transaction_mode=per-batch`.
    - `rollback-all` is most meaningful with `transaction_mode=per-run`.
    - With `transaction_mode=none`, there is no explicit transaction to roll back, so rollback policies mainly affect whether processing stops or continues.
- `rate_limit_rows_per_sec`: maximum import speed target after successful writes. Default `20000`.
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
- `config`: object. Generator-specific config. Use `data-generator info <name>` for details.
- `unique`: boolean. Only for generators that support unique output.
- `data_linkage`: string. Format `module:Group1` (module depends on generator).
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

Use `data-generator info <name>` to see required config keys and supported values for each generator.

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
data-generator check example_mysql_db.json

# Preview one row (format comes from JSON config)
data-generator preview example_file.json

# Generate CSV to file
data-generator run example_file.json --output out.csv

# Generate data into MySQL (ODBC)
data-generator run example_mysql_db.json
```

**License**

MIT. See `LICENSE`.
