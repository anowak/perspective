// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ ██████ ██████ ██████       █      █      █      █      █ █▄  ▀███ █       ┃
// ┃ ▄▄▄▄▄█ █▄▄▄▄▄ ▄▄▄▄▄█  ▀▀▀▀▀█▀▀▀▀▀ █ ▀▀▀▀▀█ ████████▌▐███ ███▄  ▀█ █ ▀▀▀▀▀ ┃
// ┃ █▀▀▀▀▀ █▀▀▀▀▀ █▀██▀▀ ▄▄▄▄▄ █ ▄▄▄▄▄█ ▄▄▄▄▄█ ████████▌▐███ █████▄   █ ▄▄▄▄▄ ┃
// ┃ █      ██████ █  ▀█▄       █ ██████      █      ███▌▐███ ███████▄ █       ┃
// ┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
// ┃ Copyright (c) 2017, the Perspective Authors.                              ┃
// ┃ ╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌ ┃
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
    static t_udf_registry& get() {
        static t_udf_registry instance;
        return instance;
    }

    void register_reducer(std::string name, t_udf_reducer reducer) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_reducers[std::move(name)] = std::move(reducer);
    }

    t_udf_reducer get_reducer(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto iter = m_reducers.find(name);
        if (iter == m_reducers.end()) {
            return nullptr;
        }
        return iter->second;
    }

    void load_plugins_from_env() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_plugins_loaded) {
            return;
        }
        m_plugins_loaded = true;

        const char* env = std::getenv("PERSPECTIVE_UDF_PLUGINS");
        if (!env || env[0] == '\0') {
            return;
        }

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

#ifdef _WIN32
            HMODULE handle = LoadLibraryA(path.c_str());
            if (!handle) {
                PSP_COMPLAIN("Unable to load UDF plugin: " << path);
                continue;
            }
            auto reducer_reg = reinterpret_cast<t_udf_registration_fn>(
                GetProcAddress(handle, "perspective_register_udf_reducers")
            );
#else
            void* handle = dlopen(path.c_str(), RTLD_LAZY);
            if (!handle) {
                PSP_COMPLAIN("Unable to load UDF plugin: " << path);
                continue;
            }
            auto reducer_reg = reinterpret_cast<t_udf_registration_fn>(
                dlsym(handle, "perspective_register_udf_reducers")
            );
#endif
            if (!reducer_reg) {
                PSP_COMPLAIN(
                    "Missing registration symbol in UDF plugin (expected "
                    "perspective_register_udf_reducers): "
                    << path
                );
                continue;
            }

            reducer_reg();
            m_plugin_handles.push_back(reinterpret_cast<void*>(handle));
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

