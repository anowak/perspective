# UDF plugin example

This example shows how to package reducer and combiner UDFs in a shared library
and use them from a Perspective application without modifying the main source
code.

The sample plugin registers two callbacks:

- `udf_reducer_running_total` – a reducer that sums every `value` column it
  sees.
- `udf_combiner_full_name` – a combiner that joins `first_name` and
  `last_name` pairs together.

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
           running_total: "udf_reducer_running_total",
           full_name: "udf_combiner_full_name"
       }
   });
   ```

Perspective will load the plugin and execute the registered callbacks whenever
the UDF aggregations are evaluated.
