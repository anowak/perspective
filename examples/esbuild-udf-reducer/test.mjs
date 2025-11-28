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

console.log("UDF reducer aggregation verified.");
