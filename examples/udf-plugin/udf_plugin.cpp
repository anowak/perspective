#include <perspective/udf_registry.h>

#include <string>

using namespace perspective;

extern "C" void perspective_register_udf_reducers() {
    register_udf_reducer("running_total", [](std::vector<t_tscalar>& values) {
        if (values.empty()) {
            t_tscalar none;
            none.clear();
            return none;
        }

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
