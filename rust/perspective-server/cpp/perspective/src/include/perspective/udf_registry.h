// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃ ██████ ██████ ██████       █      █      █      █      █ █▄  ▀███ █       ┃
// ┃ ▄▄▄▄▄█ █▄▄▄▄▄ ▄▄▄▄▄█  ▀▀▀▀▀█▀▀▀▀▀ █ ▀▀▀▀▀█ ████████▌▐███ ███▄  ▀█ █ ▀▀▀▀▀ ┃
// ┃ █▀▀▀▀▀ █▀▀▀▀▀ █▀██▀▀ ▄▄▄▄▄ █ ▄▄▄▄▄█ ▄▄▄▄▄█ ████████▌▐███ █████▄   █ ▄▄▄▄▄ ┃
// ┃ █      ██████ █  ▀█▄       █ ██████      █      ███▌▐███ ███████▄ █       ┃
// ┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫
// ┃ Copyright (c) 2017, the Perspective Authors.                              ┃
// ┃ ╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌ ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛

#pragma once

#include <perspective/exports.h>
#include <perspective/scalar.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace perspective {

using t_udf_reducer = std::function<t_tscalar(std::vector<t_tscalar>&)>;

/**
 * Register a reducer UDF by name. Users are expected to call this from a plugin
 * or embeddor binary before executing a query that references
 * `AGGTYPE_UDF_REDUCER` aggregations named `udf_reducer_<name>`.
 */
PERSPECTIVE_EXPORT void register_udf_reducer(
    const std::string& name,
    t_udf_reducer reducer
);

/**
 * Retrieve a reducer UDF by name. Returns `nullptr` if the reducer is not
 * available.
 */
PERSPECTIVE_EXPORT t_udf_reducer get_udf_reducer(const std::string& name);

/**
 * Load reducer UDF plugins described by the environment variable
 * `PERSPECTIVE_UDF_PLUGINS`.
 */
PERSPECTIVE_EXPORT void load_udf_plugins_from_env();

/**
 * Utility for plugin shared libraries: when Perspective loads a plugin, it
 * looks for a symbol with this name and executes it so the plugin can register
 * its reducers.
 */
using t_udf_registration_fn = void (*)();

} // namespace perspective

