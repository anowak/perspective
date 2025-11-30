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

#ifdef PSP_STATIC_UDF
extern "C" void perspective_register_udf_reducers();
#endif

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
        register_reducer(
            std::string name,
            t_udf_reducer reducer,
            std::vector<t_dtype> input_types
        ) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_reducers[std::move(name)] = t_udf_reducer_info{
                name, std::move(input_types), std::move(reducer)
            };
        }

        t_udf_reducer
        get_reducer(const std::string& name) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto iter = m_reducers.find(name);
            if (iter == m_reducers.end()) {
                return nullptr;
            }
            return iter->second.reducer;
        }

        std::vector<t_udf_reducer_info>
        list_reducers() {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::vector<t_udf_reducer_info> reducers;
            reducers.reserve(m_reducers.size());
            for (const auto& reducer : m_reducers) {
                reducers.push_back(reducer.second);
            }
            return reducers;
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

#ifdef PSP_STATIC_UDF
            perspective_register_udf_reducers();
#endif

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
        }

        std::unordered_map<std::string, t_udf_reducer_info> m_reducers;
        std::mutex m_mutex;
        bool m_plugins_loaded = false;
        std::vector<void*> m_plugin_handles;
    };

} // namespace

void
register_udf_reducer(
    const std::string& name,
    t_udf_reducer reducer,
    std::vector<t_dtype> input_types
) {
    t_udf_registry::get().register_reducer(
        name, std::move(reducer), std::move(input_types)
    );
}

t_udf_reducer
get_udf_reducer(const std::string& name) {
    return t_udf_registry::get().get_reducer(name);
}

std::vector<t_udf_reducer_info>
get_registered_udf_reducers() {
    return t_udf_registry::get().list_reducers();
}

void
load_udf_plugins_from_env() {
    t_udf_registry::get().load_plugins_from_env();
}

} // namespace perspective
