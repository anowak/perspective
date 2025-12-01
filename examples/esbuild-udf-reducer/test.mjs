// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ ██████ ██████ ██████       █      █      █      █      █ █▄  ▀███ █       ┃
// ┃ ▄▄▄▄▄█ █▄▄▄▄▄ ▄▄▄▄▄█  ▀▀▀▀▀█▀▀▀▀▀ █ ▀▀▀▀▀█ ████████▌▐███ ███▄  ▀█ █ ▀▀▀▀▀ ┃
// ┃ █▀▀▀▀▀ █▀▀▀▀▀ █▀██▀▀ ▄▄▄▄▄ █ ▄▄▄▄▄█ ▄▄▄▄▄█ ████████▌▐███ █████▄   █ ▄▄▄▄▄ ┃
// ┃ █      ██████ █  ▀█▄       █ ██████      █      ███▌▐███ ███████▄ █       ┃
// ┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
// ┃ Copyright (c) 2017, the Perspective Authors.                              ┃
// ┃ ╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌ ┃
// ┃ This file is part of the Perspective library, distributed under the terms ┃
// ┃ of the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0). ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛

import assert from "node:assert/strict";
import perspective from "@perspective-dev/client";

const websocket = await perspective.websocket("ws://localhost:8081/subscribe");
const table = await websocket.open_table("securities");

const view = await table.view({
    group_by: ["name"],
    columns: ["client"],
    sort: [
        ["name", "asc"],
    ],
    aggregates: {
        name: "first",
        client: "udf_reducer_join_lines",
    },
});

const result = await view.to_columns();
await view.delete();

// The first row is the grand total; skip it and compare grouped rows.
const rows = result.__ROW_PATH__
    .map((path, idx) => ({
        name: path[0],
        client: result.client[idx],
    }))
    .filter((row) => row.name);

const expected = [
    { name: "AAPL.N", client: "Ada\nBen" },
    { name: "AMZN.N", client: "Ada\nCara" },
    { name: "NVDA.N", client: "Ben\nDana" },
];

assert.deepEqual(rows, expected);

// Add duplicate rows to verify the deduping reducer keeps first-seen order.
await table.update([
    { name: "AAPL.N", client: "Ada" },
    { name: "AAPL.N", client: "Cara" },
    { name: "NVDA.N", client: "Ben" },
    { name: "NVDA.N", client: "Ada" },
]);

const view2 = await table.view({
    group_by: ["name"],
    columns: ["client"],
    sort: [
        ["name", "asc"],
    ],
    aggregates: {
        name: "first",
        client: ["udf_reducer_join_unique_lines", ["client"]],
    },
});

const result2 = await view2.to_columns();
await view2.delete();

const rows2 = result2.__ROW_PATH__
    .map((path, idx) => ({
        name: path[0],
        client: result2.client[idx],
    }))
    .filter((row) => row.name);

const expected2 = [
    { name: "AAPL.N", client: "Ada\nBen\nCara" },
    { name: "AMZN.N", client: "Ada\nCara" },
    { name: "NVDA.N", client: "Ben\nDana\nAda" },
];

assert.deepEqual(rows2, expected2);

console.log("UDF reducer aggregation verified for join_lines and join_unique_lines.");
