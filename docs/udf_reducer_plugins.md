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
    register_udf_reducer("join_lines", [](std::vector<t_tscalar>& values) {
        std::string out;
        bool first = true;

        for (const auto& value : values) {
            if (!value.is_valid()) {
                continue;
            }

            std::string str = value.to_string();
            if (str.empty()) {
                continue;
            }

            if (!first) {
                out += "\n";
            }

            out += str;
            first = false;
        }

        t_tscalar result;
        if (first) {
            result.clear();
        } else {
            result.set(out);
        }

        return result;
    });
}
```

The reducer receives the `std::vector<t_tscalar>` values for the aggregation's
single dependency column and returns a `t_tscalar` result.

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

