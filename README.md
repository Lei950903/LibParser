# Liberty Parser & STA Prototype Tool

This project is a C++17 prototype for parsing Liberty timing-library files and
performing basic static timing analysis (STA).

The implementation is intentionally limited to a small Liberty subset and is
intended for learning and experimentation rather than production signoff.

## Implemented Features

- Parse line comments (`//`) and block comments (`/* ... */`)
- Parse the following Liberty block types:
	- `library`
	- `lu_table_template`
	- `cell`
	- `pin`
	- `timing`
- Parse two-dimensional lookup-table data:
	- `index_1`
	- `index_2`
	- `values`
- Parse timing tables:
	- `cell_rise`
	- `cell_fall`
	- `rise_transition`
	- `fall_transition`
- Perform bilinear interpolation for timing-table values
- Accumulate delay across configured single-segment and multi-segment paths
- Calculate basic setup and hold slack and report whether each check is met
- Use `config.cfg` as a lightweight replacement for standard SDC constraints
- Separate parser, configuration-loader, and path-analyzer components

## Prototype Limitations

- The parser supports only the Liberty constructs implemented in the source
	code; it is not a complete Liberty grammar.
- Standard SDC parsing is not implemented. Timing paths and analysis
	parameters are configured manually through `config.cfg` instead.
- The `config.cfg` format is a project-specific configuration format, not an
	SDC-compatible format.
- Additional path segments currently reuse the global transition and load
	values from the configuration file.
- Setup and hold checks use simplified equations. Clock uncertainty and other
	signoff effects are not currently included in the calculation.
- Lookup-table inputs are intended to be within the table's index range. Input
	values outside the range are currently extrapolated by the interpolation
	routine and should be treated as prototype behavior.

## Build and Run

```bash
make clean
make
./LibertyParser
```

The executable expects `simple.lib` and `config.cfg` in the current working
directory.