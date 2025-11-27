# Custom reducer UDF plugins

`AGGTYPE_UDF_REDUCER` can now be supplied by external plugins instead of
modifying Perspective's core source. Plugins are shared libraries that register
reducers under a name (e.g. `my_sum`), which can then be referenced from view
configurations as `udf_reducer_my_sum`.

## Authoring a plugin

1. Build a shared library that links against Perspective headers.
2. Export a `perspective_register_udf_reducers` symbol. Perspective invokes this
   function when the plugin is loaded so you can call `register_udf_reducer`.

```cpp
#include <perspective/udf_registry.h>

using namespace perspective;

extern "C" void perspective_register_udf_reducers() {
    register_udf_reducer("my_sum", [](const t_udf_column_values& columns) {
        const auto& values = columns.at("column");
        t_tscalar total;
        total.set(std::int64_t(0));
        for (const auto& value : values) {
            if (value.is_valid() && !value.is_nan()) {
                total = total.add(value);
            }
        }
        return total;
    });
}
```

The `t_udf_column_values` map includes one entry per dependency declared on the
aggregation, keyed by dependency name.

## Loading plugins

Set the `PERSPECTIVE_UDF_PLUGINS` environment variable to a colon-separated list
of plugin paths (or semicolon-separated on Windows) before constructing a
`Context`. Plugins are lazily loaded the first time a reducer aggregation is
evaluated.

```bash
export PERSPECTIVE_UDF_PLUGINS="/path/to/libmy_udfs.so:/path/to/other.so"
```

If no reducer is registered for the requested `disp_name`, Perspective will
raise an error rather than silently returning `null`.

