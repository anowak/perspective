#include <perspective/udf_registry.h>

#include <string>

using namespace perspective;

extern "C" void perspective_register_udf_reducers() {
    register_udf_reducer("running_total", [](const t_udf_column_values& columns) {
        const auto& values = columns.at("value");
        t_tscalar running;
        running.set(std::int64_t(0));

        for (const auto& item : values) {
            if (item.is_valid() && !item.is_nan()) {
                running = running.add(item);
            }
        }

        return running;
    });
}

extern "C" void perspective_register_udf_combiners() {
    register_udf_combiner("full_name", [](const t_udf_column_values& columns) {
        const auto& first = columns.at("first_name");
        const auto& last = columns.at("last_name");

        std::string combined;
        combined.reserve(first.size() * 8);

        for (std::size_t idx = 0; idx < first.size(); ++idx) {
            if (!first[idx].is_valid() || !last[idx].is_valid()) {
                continue;
            }

            if (!combined.empty()) {
                combined.append(", ");
            }

            combined.append(first[idx].to_string());
            combined.push_back(' ');
            combined.append(last[idx].to_string());
        }

        t_tscalar result;
        result.set(combined);
        return result;
    });
}
