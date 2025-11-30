# `esbuild` reducer UDF example

This example builds on the `esbuild-remote` sample to demonstrate invoking a
custom reducer UDF from a browser bundle. A shared library plugin registers a
reducer (`udf_reducer_join_lines`) that concatenates string values with newline
separators. The Node server loads the plugin via `PERSPECTIVE_UDF_PLUGINS`, and
the browser client requests the UDF aggregate over a WebSocket session.

## Prerequisites

1. Build the reducer plugin in [`../udf-plugin`](../udf-plugin/README.md).
2. Export the plugin path before starting the server, or rely on the default
   baked into `pnpm start`/`server/index.mjs`:

   ```bash
   export PERSPECTIVE_UDF_PLUGINS=${PWD}/../udf-plugin/build/libperspective_udfs.wasm
   ```

## Running the demo

From this directory:

```bash
pnpm install
pnpm start
```

The `start` script bundles the client with `esbuild` and starts the Node server
on port `8081`. Visit http://localhost:8081 to see the newline-joined client
names per security in the `<perspective-viewer>` grid. The browser bundle sets
the `udf_reducer_join_lines` aggregate on the `client` column so you can see the
plugin result immediately once the server streams data.

## Verifying the reducer from Node

With the server running, you can exercise the UDF reducer without a browser:

```bash
pnpm --filter esbuild-udf-reducer test
```

For deterministic output, start the server with static fixture rows:

```bash
PERSPECTIVE_UDF_STATIC=1 PERSPECTIVE_UDF_PLUGINS=${PWD}/../udf-plugin/build/libperspective_udfs.so pnpm start
```

The test connects over WebSocket and asserts that the reducer returns a
newline-separated list of client names for each security.
