#include <perspective/udf_registry.h>

#include <string>

using namespace perspective;

extern "C" void perspective_register_udf_reducers() {
    register_udf_reducer("running_total", [](const t_udf_column_values& columns) {
        if (columns.empty()) {
            t_tscalar none;
            none.clear();
            return none;
        }

        auto value_iter = columns.find("value");
        const auto& values =
            value_iter == columns.end() ? columns.begin()->second : value_iter->second;
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
