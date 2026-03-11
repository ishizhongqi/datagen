# Data Generator

[![codecov](https://codecov.io/gh/ishizhongqi/data-generator/branch/develop/graph/badge.svg?token=TJZIICPRO1)](https://codecov.io/gh/ishizhongqi/data-generator)

Data Generator is a C++ CLI that generates synthetic datasets from a JSON configuration. It can write CSV/JSON/SQL files or insert rows into databases via ODBC.

**Build**

```sh
cmake -S . -B build
cmake --build build
```

The binary is named `data-generator`. If you build with the commands above, run it as `./build/data-generator` or add it to your `PATH`.

**Usage**

**CLI**

`data-generator <command> [options]`

Command summary:

- `help`: Show help for all commands.
- `init`: Generate a JSON configuration template.
- `preview`: Generate a single-row preview.
- `generate`: Generate full dataset from JSON config.
- `describe`: Describe a generator and its config.
- `list`: List available generators.
- `drivers`: List installed ODBC drivers.
- `validate`: Validate a JSON config, optionally DB-only.
- `schema`: Print or write the JSON Schema.

**Command: `init`**

Options:

- `--generator <name>`: Create a template with a single generator field. Supported values: generator names listed below.
- `--output <file>`: Output JSON file path. Default `config_YYYYmmddHHMMSS.json`.
- `--rows <N>`: Rows to generate. Integer `>= 1`.
- `--destination <file|database>`: Output destination. `file` or `database`. Default `file`.
- `--file-format <csv|json|sql>`: File output format. Only valid when `destination=file`.
- `--database-url <url>`: Database URL or ODBC connection string. Used when `destination=database`.
- `--table <name>`: Target table. Used when `destination=database` or `file-format=sql`.
- `-h, --help`: Show help.

**Command: `preview`**

Options:

- `--input <json>`: Input JSON config file. Required.
- `--field <name|all>`: Field name to preview. Default `all`.
- `--file-format <csv|json|sql>`: Preview output format. Optional override.
- `-h, --help`: Show help.

**Command: `generate`**

Options:

- `--input <json>`: Input JSON config file. Required.
- `--file-output <file>`: Output file path (file destination). If omitted, writes `dgresult_YYYYmmddHHMMSS.<format>`.
- `--output <file>`: Alias of `--file-output`.
- `--rows <N>`: Override rows in JSON. Integer `>= 1`.
- `--file-format <csv|json|sql>`: Override file format. Ignored when `destination=database`.
- `--destination <file|database>`: Override output destination. `file` or `database`.
- `--database-url <url>`: Database URL or ODBC connection string. Required for database output if missing in JSON.
- `--table <name>`: Target table. Must be non-empty.
- `--threads <N>`: Generator threads for eligible workloads. Integer `>= 1`, default `1`.
- `-h, --help`: Show help.

**Command: `describe`**

Options:

- `--generator <name>`: Generator name to describe. Required.
- `--json`: Output JSON instead of text.
- `-h, --help`: Show help.

**Command: `list`**

Options:

- `-h, --help`: Show help.

**Command: `drivers`**

Options:

- `--json`: Output JSON instead of plain text.
- `-h, --help`: Show help.

**Command: `validate`**

Options:

- `--input <json>`: Input JSON config file. Required.
- `--db-only`: Validate database connection only. Requires `database_url`.
- `-h, --help`: Show help.

**Command: `schema`**

Options:

- `--output <file>`: Write schema to file instead of stdout.
- `-h, --help`: Show help.

**JSON Configuration**

Root keys:

- `$schema`: string. JSON Schema path.
- `rows`: integer. Required for `generate`. `>= 1`.
- `destination`: string. `file` or `database`. Default `file`.
- `file_format`: string. Required for `destination=file`. `csv`, `json`, or `sql`. Ignored for `destination=database`.
- `null_value_string`: string or null. `null` means empty output for nulls; string means literal value for nulls.
- `table`: string. Required for `sql` or database output.
- `database_url`: string. Required for `destination=database`.
- `database_insert_mode`: string. `auto`, `insert`, `bulk`, `load`.
- `database_batch_size`: integer. `>= 1`.
- `database_queue_size`: integer. `>= 1`.
- `database_threads`: integer. `>= 1`.
- `database_transaction_mode`: string. `per-batch`, `per-run`, `none`.
- `database_error_policy`: string. `stop`, `continue`, `rollback-batch`, `rollback-all`.
- `database_rate_limit_rows_per_sec`: integer. `>= 1`.
- `fields`: array. Required. Array of field objects.

Field object keys:

- `name`: string. Column/field name.
- `generator`: string. See supported generators below.
- `config`: object. Generator-specific config. Use `data-generator describe --generator <name>` for details.
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

Use `data-generator describe --generator <name>` to see required config keys and supported values for each generator.

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
./build/data-generator validate --input docs/example_mysql_db.json

# Preview one row as JSON
./build/data-generator preview --input docs/example_file.json --file-format json

# Generate CSV to file
./build/data-generator generate --input docs/example_file.json --file-output ./out.csv --file-format csv

# Generate data into MySQL (ODBC)
./build/data-generator generate --input docs/example_mysql_db.json --destination database \
  --database-url "odbc:mysql:DRIVER={MySQL ODBC 8.0 Unicode Driver};SERVER=127.0.0.1;PORT=3306;DATABASE=example_db;UID=user;PWD=password;" \
  --table customer_profile
```

**License**

MIT. See `LICENSE`.
