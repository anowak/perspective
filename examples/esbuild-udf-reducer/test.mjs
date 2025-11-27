import assert from "node:assert/strict";
import perspective from "@perspective-dev/client";

const websocket = await perspective.websocket("ws://localhost:8081/subscribe");
const table = await websocket.open_table("securities");

const view = await table.view({
    group_by: ["client"],
    columns: ["vol"],
    aggregates: {
        client: "first",
        vol: "udf_reducer_running_total",
    },
});

const result = await view.to_columns();
await view.delete();
await websocket.close();

// Each client should report the running total for all rows in its group.
const expected = {
    client: ["CLIENT_1", "CLIENT_2", "CLIENT_3"],
    vol: ["817727", "759594", "489467"],
};

assert.deepEqual(result, expected);

console.log("UDF reducer aggregation verified.");
