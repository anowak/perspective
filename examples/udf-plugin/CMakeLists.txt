cmake_minimum_required(VERSION 3.16)
project(perspective_udf_plugin CXX)

if(NOT DEFINED PERSPECTIVE_ROOT)
  message(FATAL_ERROR "PERSPECTIVE_ROOT must point to rust/perspective-server/cpp/perspective")
endif()

if(NOT DEFINED PERSPECTIVE_BUILD_DIR)
  # Default to the release build output from rust/perspective-server/build
  set(PERSPECTIVE_BUILD_DIR ${PERSPECTIVE_ROOT}/../../build/release)
endif()

set(PERSPECTIVE_INCLUDE_DIRS
  ${PERSPECTIVE_ROOT}/src/include
  ${PERSPECTIVE_ROOT}/src/include/cpp
  ${PERSPECTIVE_BUILD_DIR}/Boost-src
  ${PERSPECTIVE_BUILD_DIR}/hopscotch-src/include
  ${PERSPECTIVE_BUILD_DIR}/ordered-map-src/include
  ${PERSPECTIVE_BUILD_DIR}/rapidjson-src/include
  ${PERSPECTIVE_BUILD_DIR}/date-src/include
)
set(PERSPECTIVE_INCLUDE_FLAGS "")
foreach(dir ${PERSPECTIVE_INCLUDE_DIRS})
  list(APPEND PERSPECTIVE_INCLUDE_FLAGS "-I${dir}")
endforeach()

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # Build a WASM side-module that can be dlopened by the main Perspective
  # WASM module. Leave Perspective symbols undefined so they are imported from
  # the host at runtime.
  set(PLUGIN_WASM ${CMAKE_CURRENT_BINARY_DIR}/libperspective_udfs.wasm)
  add_custom_command(
    OUTPUT ${PLUGIN_WASM}
    COMMAND ${CMAKE_CXX_COMPILER}
      -std=c++17 -O3 -sSIDE_MODULE=1 -sERROR_ON_UNDEFINED_SYMBOLS=0
      -fwasm-exceptions
      -o ${PLUGIN_WASM}
      udf_plugin.cpp
      ${PERSPECTIVE_INCLUDE_FLAGS}
    DEPENDS udf_plugin.cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Linking WASM UDF plugin"
  )
  add_custom_target(perspective_udfs ALL DEPENDS ${PLUGIN_WASM})
else()
  add_library(perspective_udfs SHARED udf_plugin.cpp)
  target_include_directories(perspective_udfs PRIVATE ${PERSPECTIVE_INCLUDE_DIRS})
  target_link_libraries(perspective_udfs PRIVATE psp)
  set_target_properties(perspective_udfs PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
  )

  # Let consumers know where the Perspective libs live when building locally
  if(WIN32)
    target_link_directories(perspective_udfs PRIVATE ${PERSPECTIVE_ROOT}/build/Release)
  else()
    target_link_directories(perspective_udfs PRIVATE ${PERSPECTIVE_BUILD_DIR})
  endif()
endif()
