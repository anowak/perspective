#include <perspective/udf_registry.h>

#include <string>

using namespace perspective;

extern "C" void perspective_register_udf_reducers() {
    register_udf_reducer("join_lines", [](std::vector<t_tscalar>& values) {
        std::string joined;
        bool first = true;

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
            result.set(joined);
        }

        return result;
    });
}
