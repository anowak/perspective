# UDF plugin example

This example shows how to package reducer UDFs in a shared library and use them
from a Perspective application without modifying the main source code.

The sample plugin registers one callback that consumes a single dependency's
values:

- `udf_reducer_join_lines` – concatenates string values into a single newline-
  separated block when the aggregation uses `dependencies: ["client"]` (or any
  other string column).

## Building the plugin

The plugin is a tiny CMake project that links against the Perspective C++ core
built in `rust/perspective-server/cpp/perspective`. After building Perspective,
run the following from this directory:

```bash
cmake -S . -B build \
  -DPERSPECTIVE_ROOT=${PWD}/../../rust/perspective-server/cpp/perspective
cmake --build build
```

This produces `build/libperspective_udfs.so` (or `.dll`/`.dylib` depending on
platform).

## Running with a local application

1. Export the plugin path so Perspective can load it lazily:

   ```bash
   export PERSPECTIVE_UDF_PLUGINS=${PWD}/build/libperspective_udfs.so
   ```

2. Start any Perspective-powered binary (for example the Rust Axum demo in
   `examples/rust-axum`) in a separate shell with the same environment.

3. Reference the UDFs from a view configuration:

   ```js
   const view = await table.view({
       group_by: ["city"],
       aggregates: {
           city: "count",
           clients: "udf_reducer_join_lines"
       }
   });
   ```

Perspective will load the plugin and execute the registered callbacks whenever
the UDF aggregations are evaluated.
