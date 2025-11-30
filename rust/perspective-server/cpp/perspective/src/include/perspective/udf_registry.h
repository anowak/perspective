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

#pragma once

#include <perspective/exports.h>
#include <perspective/base.h>
#include <perspective/scalar.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace perspective {

using t_udf_reducer = std::function<t_tscalar(std::vector<t_tscalar>&)>;

struct t_udf_reducer_info {
    std::string name;
    std::vector<t_dtype> input_types;
    t_udf_reducer reducer;
};

/**
 * Register a reducer UDF by name. Users are expected to call this from a plugin
 * or embeddor binary before executing a query that references
 * `AGGTYPE_UDF_REDUCER` aggregations named `udf_reducer_<name>`.
 */
PERSPECTIVE_EXPORT void
register_udf_reducer(const std::string& name, t_udf_reducer reducer);

/**
 * Register a reducer UDF with additional metadata. `input_types` limits the
 * column types the reducer can be applied to (empty means all), and
 * `display_name` customizes how the reducer is shown in the UI (defaults to
 * the registered name).
 */
PERSPECTIVE_EXPORT void register_udf_reducer(
    const std::string& name,
    t_udf_reducer reducer,
    std::vector<t_dtype> input_types
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
 * List the reducers registered in the current process along with their
 * metadata.
 */
PERSPECTIVE_EXPORT std::vector<t_udf_reducer_info>
get_registered_udf_reducers();

/**
 * Utility for plugin shared libraries: when Perspective loads a plugin, it
 * looks for a symbol with this name and executes it so the plugin can register
 * its reducers.
 */
using t_udf_registration_fn = void (*)();

} // namespace perspective
