# CMake to build libraries and binaries in fboss/cli/fboss2

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  fsdb_common_cpp2
  fboss/fsdb/if/fsdb_common.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  fsdb_oper_cpp2
  fboss/fsdb/if/fsdb_oper.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    fsdb_common_cpp2
)


add_fbthrift_cpp_library(
  fsdb_cpp2
  fboss/fsdb/if/fsdb.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    fsdb_common_cpp2
    fsdb_oper_cpp2
)

add_fbthrift_cpp_library(
  fsdb_model_cpp2
  fboss/fsdb/if/oss/fsdb_model.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    agent_config_cpp2
    agent_stats_cpp2
    switch_state_cpp2
    qsfp_state_cpp2
    qsfp_stats_cpp2
    sensor_service_stats_cpp2
)

add_library(thriftpath_lib
  fboss/thriftpath_plugin/Path.cpp
)

target_link_libraries(thriftpath_lib
  FBThrift::thriftcpp2
  Folly::folly
)

add_library(fsdb_model_thriftpath_cpp2
  fboss/fsdb/if/oss/fsdb_model_thriftpath.h
)

target_link_libraries(fsdb_model_thriftpath_cpp2
  thriftpath_lib
  fsdb_model_cpp2
)
