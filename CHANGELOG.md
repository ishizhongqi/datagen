# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-07-13

### Added

- Initial stable release of the `datagen` C++ CLI for generating synthetic datasets from JSON configuration files.
- File output support for CSV, JSON, SQL, tab-delimited, and custom-delimited formats.
- Database output support for ODBC and SQLite targets with schema validation, batching, transactions, and configurable error handling.
- CLI commands for initialization, previewing, generation, generator metadata, driver listing, configuration checks, and JSON Schema generation.
- Generator catalog backed by `faker`, including business, computer, datetime, location, number, payment, person, product, string, and utility generators.
- Cross-platform CI workflows for GCC, Clang, Apple Clang, MSVC, and coverage builds.
- Documentation, example configurations, database schema samples, and unit tests for core generation, CLI, file output, and database output behavior.
