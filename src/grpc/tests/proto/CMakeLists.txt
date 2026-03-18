cmake_minimum_required(VERSION 3.11)

# Find the grpc_cpp_plugin
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin PATHS ENV PATH)
if(NOT GRPC_CPP_PLUGIN)
  set(GRPC_CPP_PLUGIN "/home/ubuntu/.conan2/p/b/grpc894ee3c31ed7e/p/bin/grpc_cpp_plugin")
endif()

add_library(test_proto OBJECT)
target_sources(test_proto PRIVATE
  grpc_helper_test.proto
  )
target_link_libraries(test_proto PUBLIC
  gRPC::grpc++
  )

protobuf_generate(LANGUAGE cpp TARGET test_proto PROTOS)
protobuf_generate(
  TARGET test_proto
  LANGUAGE grpc
  GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
  PLUGIN protoc-gen-grpc=${GRPC_CPP_PLUGIN}
  )
