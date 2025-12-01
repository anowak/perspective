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

#include <perspective/udf_registry.h>

#include <iostream>
#include <string>
#include <unordered_set>

using namespace perspective;

extern "C" void perspective_register_udf_reducers() {
    register_udf_reducer("join_lines", [](std::vector<t_tscalar>& values) {
        std::string joined;
        bool first = true;
        static int call_id = 0;
        ++call_id;
        std::cerr << "[join_lines] call #" << call_id << " values=" << values.size()
                  << std::endl;

        for (const auto& value : values) {
            if (!value.is_valid() || value.is_nan()) {
                continue;
            }

            std::string as_string = value.to_string();
            if (as_string.empty()) {
                continue;
            }

            if (!first) {
                joined += "\n";
            }

            joined += as_string;
            first = false;
        }

        t_tscalar result;
        if (first) {
            result.clear();
        } else {
            char* buffer = strdup(joined.c_str());
            result.set(buffer);
        }

        return result;
    }, {t_dtype::DTYPE_STR});

    register_udf_reducer("join_unique_lines", [](std::vector<t_tscalar>& values) {
        std::string joined;
        bool first = true;
        std::unordered_set<std::string> seen;
        seen.reserve(values.size());
        static int call_id = 0;
        ++call_id;
        std::cerr << "[join_unique_lines] call #" << call_id
                  << " values=" << values.size() << std::endl;

        for (const auto& value : values) {
            if (!value.is_valid() || value.is_nan()) {
                continue;
            }

            std::string as_string = value.to_string();
            if (as_string.empty()) {
                continue;
            }

            if (!seen.insert(as_string).second) {
                continue;
            }

            if (!first) {
                joined += "\n";
            }

            joined += as_string;
            first = false;
        }

        t_tscalar result;
        if (first) {
            result.clear();
        } else {
            char* buffer = strdup(joined.c_str());
            result.set(buffer);
        }

        return result;
    }, {t_dtype::DTYPE_STR});
}
