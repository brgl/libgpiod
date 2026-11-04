# Changelog

All notable changes to this project will be documented in this file.

The format is loosely based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [v2.6.0]

### Added

- Advertise support for Python 3.15
- Generate wheels for CPython 3.15

### Changed

- Bump the `ruff` linter dependency to v0.17 and simplify its configuration
- (Internal) Include `Python.h` before other headers in the C extension to
  avoid redefinition warnings
- (Internal) Bump build dependencies in the artifact generation script
- (Internal) Use literals for dict/list and add type annotations in `Chip`
- (Internal) Drop redundant exception in re-raise in `Chip`
- (Internal) Simplify `__bool__` expressions in `Chip` and `LineRequest`
- (Internal) Sort exports in `line` and `_internal` modules
- (Internal) Simplify `poll_fd` calculation and collapse `isinstance` checks
- (Internal) Use a context manager to manage files in `setup.py`

### Fixed

- Fix source directory validation in the artifact generation script
- Verify that the output directory in the artifact generation script is
  actually a directory

## [v2.5.0]

### Added

- Add free-threaded CPython support
- Generate wheels for CPython 3.14t (free-threaded)
- Include the license file into the source and binary distributions

### Changed

- Remove `wheel` from build dependencies
- Allow `Chip.close` to be called multiple times without raising an error
- Allow `LineRequest.release` to be called multiple times without raising an error
- (Internal) Add more patterns to `.gitignore`
- (Internal) Update linter configuration
- (Internal) Refactor code to conform to updated linter configuration
- (Internal) Refactor code to use Python 3.10 type annotations
- (Internal) Introduce a dependency group for linting
- (Internal) Simplify Chip finalization
- (Internal) Simplify LineRequest finalization
- (Internal) Use newer functions/macros in the C extension.
- (Internal) Migrate to multi-phase module initialization
- (Internal) Removed duplicate call to `gpiod_line_settings_set_edge_detection`
- (Internal) Add multi-threaded tests

### Fixed

- Fix refcount imbalance due to missing `Py_INCREF` when querying line names
- Fix refcount imbalance by not calling `Py_DECREF` when `PyList_SetItem` fails
- Require `setuptools` v77 or higher to avoid build errors

### Security

- Fix possible buffer overflows when setting/getting `LineRequest` values

### Removed

- Support for Python 3.9 has been dropped
