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

#include <perspective/first.h>
#include <perspective/udf_registry.h>

#include <cstdlib>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
#endif

namespace perspective {

namespace {

    struct t_udf_registry {
        static t_udf_registry&
        get() {
            static t_udf_registry instance;
            return instance;
        }

        void
        register_reducer(std::string name, t_udf_reducer reducer) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_reducers[std::move(name)] = std::move(reducer);
        }

        t_udf_reducer
        get_reducer(const std::string& name) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_reducers.find(name);
            if (iter == m_reducers.end()) {
                return nullptr;
            }
            return iter->second;
        }

        void
        load_plugins_from_env() {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_plugins_loaded) {
                    return;
                }
                m_plugins_loaded = true;
            }

            const char* env = std::getenv("PERSPECTIVE_UDF_PLUGINS");
            std::vector<std::string> plugin_paths;
            if (env && env[0] != '\0') {
                std::stringstream paths(env);
#ifdef _WIN32
                const char delimiter = ';';
#else
                const char delimiter = ':';
#endif
                std::string path;
                while (std::getline(paths, path, delimiter)) {
                    if (path.empty()) {
                        continue;
                    }
                    plugin_paths.push_back(path);
                }
            }

            for (const auto& plugin_path : plugin_paths) {
#ifdef _WIN32
                HMODULE handle = LoadLibraryA(plugin_path.c_str());
                if (!handle) {
                    PSP_COMPLAIN("Unable to load UDF plugin: " << plugin_path);
                    continue;
                }
                auto reducer_reg = reinterpret_cast<t_udf_registration_fn>(
                    GetProcAddress(handle, "perspective_register_udf_reducers")
                );
#else
                void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
                if (!handle) {
                    PSP_COMPLAIN_AND_ABORT(
                        std::string("Unable to load UDF plugin: ") + plugin_path
                    );
                    continue;
                }
                auto reducer_reg = reinterpret_cast<t_udf_registration_fn>(
                    dlsym(handle, "perspective_register_udf_reducers")
                );
#endif
                if (!reducer_reg) {
                    PSP_COMPLAIN_AND_ABORT(
                        "Missing registration symbol in UDF plugin (expected "
                        "perspective_register_udf_reducers): "
                        + plugin_path
                    );
                    continue;
                }

                reducer_reg();
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_plugin_handles.push_back(reinterpret_cast<void*>(handle));
                }
            }

            // Fallback registration for environments that cannot dynamically
            // load shared libraries (e.g. the WASM build). This mirrors the
            // sample reducer provided by the `udf-plugin` example so the UDF
            // path can be exercised even when `dlopen` is unavailable.
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_reducers.find("join_lines") == m_reducers.end()) {
                    m_reducers["join_lines"] = [](std::vector<t_tscalar>& values
                                               ) {
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
                            char* buffer = strdup(joined.c_str());
                            result.set(buffer);
                        }

                        return result;
                    };
                }
            }
        }

        std::unordered_map<std::string, t_udf_reducer> m_reducers;
        std::mutex m_mutex;
        bool m_plugins_loaded = false;
        std::vector<void*> m_plugin_handles;
    };

} // namespace

void
register_udf_reducer(const std::string& name, t_udf_reducer reducer) {
    t_udf_registry::get().register_reducer(name, std::move(reducer));
}

t_udf_reducer
get_udf_reducer(const std::string& name) {
    return t_udf_registry::get().get_reducer(name);
}

void
load_udf_plugins_from_env() {
    t_udf_registry::get().load_plugins_from_env();
}

} // namespace perspective
