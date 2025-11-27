# Custom reducer and combiner UDF plugins

`AGGTYPE_UDF_REDUCER` and `AGGTYPE_UDF_COMBINER` can now be supplied by
external plugins instead of modifying Perspective's core source. Plugins are
shared libraries that register reducers/combiners under a name (e.g. `my_sum`),
which can then be referenced from view configurations as `udf_reducer_my_sum`
or `udf_combiner_my_sum`.

## Authoring a plugin

1. Build a shared library that links against Perspective headers.
2. Export a `perspective_register_udf_reducers` and/or
   `perspective_register_udf_combiners` symbol. Perspective invokes these
   functions when the plugin is loaded so you can call `register_udf_reducer`
   and `register_udf_combiner`.

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

extern "C" void perspective_register_udf_combiners() {
    register_udf_combiner("concat_pair", [](const t_udf_column_values& columns) {
        const auto& left = columns.at("left");
        const auto& right = columns.at("right");

        std::string joined;
        for (std::size_t idx = 0; idx < left.size(); ++idx) {
            if (!left[idx].is_valid() || !right[idx].is_valid()) {
                continue;
            }
            joined.append(left[idx].to_string());
            joined.push_back('-');
            joined.append(right[idx].to_string());
            if (idx + 1 < left.size()) {
                joined.append(", ");
            }
        }

        t_tscalar out;
        out.set(joined);
        return out;
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

If no reducer or combiner is registered for the requested `disp_name`,
Perspective will raise an error rather than silently returning `null`.

