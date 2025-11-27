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
await websocket.close();

// Each security should show all clients joined with newlines (fixture rows used
// when `PERSPECTIVE_UDF_STATIC=1`).
const expected = {
    name: ["AAPL.N", "AMZN.N", "NVDA.N"],
    client: ["Ada\nBen", "Ada\nCara", "Ben\nDana"],
};

assert.deepEqual(result, expected);

console.log("UDF reducer aggregation verified.");
