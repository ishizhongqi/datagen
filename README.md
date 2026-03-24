# Data Generator

[![codecov](https://codecov.io/gh/ishizhongqi/data-generator/branch/develop/graph/badge.svg?token=TJZIICPRO1)](https://codecov.io/gh/ishizhongqi/data-generator)

Data Generator is a C++ CLI that generates synthetic datasets from a JSON configuration. It can write CSV/JSON/SQL files or insert rows into databases via ODBC or SQLite.

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
- `percent`: integer. `0` to `100`.
- `value`: string. Default value when override triggers.

`null_value` object:

- `enabled`: boolean. Whether to enable null override.
- `percent`: integer. `0` to `100`.

If both `default_value` and `null_value` are enabled, their `percent` values must sum to `<= 100`.

Supported generators:

```
company_name, department, industry, ip_address, mac_address, file_path, file_directory, file_name,
file_extension, url, hostname, date, time, datetime, address_line1, address_line2, postcode,
full_address, city, region, integer, decimal, payment_method, card_type, card_number, card_date,
first_name, last_name, full_name, gender, title, marital_status, phone_number, email, job_title,
social_network_id, product_name, product_category, color, size, barcode, enum_item, text, uuid,
sequence, regular_expression
```

Use `data-generator info <name>` to see required config keys and supported values for each generator.

**Examples**

Database schemas:

- `docs/schema_mysql.sql`
- `docs/schema_oracle.sql`
- `docs/schema_postgresql.sql`

JSON config examples:

- `docs/example_mysql_db.json`
- `docs/example_file.json`

Example commands:

```sh
# Validate config
data-generator check docs/example_mysql_db.json

# Preview one row (format comes from JSON config)
data-generator preview docs/example_file.json

# Generate CSV to file
data-generator run docs/example_file.json --output ./out.csv

# Generate data into MySQL (ODBC)
data-generator run docs/example_mysql_db.json
```

**License**

MIT. See `LICENSE`.
