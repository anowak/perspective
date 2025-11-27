cmake_minimum_required(VERSION 3.16)
project(perspective_udf_plugin CXX)

if(NOT DEFINED PERSPECTIVE_ROOT)
  message(FATAL_ERROR "PERSPECTIVE_ROOT must point to rust/perspective-server/cpp/perspective")
endif()

add_library(perspective_udfs SHARED udf_plugin.cpp)

target_include_directories(perspective_udfs PRIVATE
  ${PERSPECTIVE_ROOT}/src/include
  ${PERSPECTIVE_ROOT}/src/include/cpp
)

target_link_libraries(perspective_udfs PRIVATE psp)

set_target_properties(perspective_udfs PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
)

# Let consumers know where the Perspective libs live when building locally
if(WIN32)
  target_link_directories(perspective_udfs PRIVATE ${PERSPECTIVE_ROOT}/build/Release)
else()
  target_link_directories(perspective_udfs PRIVATE ${PERSPECTIVE_ROOT}/build)
endif()
