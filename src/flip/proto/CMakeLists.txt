cmake_minimum_required(VERSION 3.11)

# Find the grpc_cpp_plugin
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin PATHS ENV PATH)
if(NOT GRPC_CPP_PLUGIN)
  # Fallback to common locations
  set(GRPC_CPP_PLUGIN "/home/ubuntu/.conan2/p/b/grpc894ee3c31ed7e/p/bin/grpc_cpp_plugin")
endif()

add_library(flip_proto OBJECT
    flip_server.proto
    flip_spec.proto
  )
protobuf_generate(LANGUAGE cpp TARGET flip_proto PROTOS flip_spec.proto)
protobuf_generate(LANGUAGE cpp TARGET flip_proto PROTOS flip_server.proto)
protobuf_generate(
    TARGET flip_proto
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN protoc-gen-grpc=${GRPC_CPP_PLUGIN}
)
target_link_libraries(flip_proto
      protobuf::libprotobuf
      gRPC::grpc++
  )

add_custom_target(flip_py_proto ALL)
protobuf_generate(LANGUAGE python TARGET flip_py_proto PROTOS flip_server.proto)
protobuf_generate(LANGUAGE python TARGET flip_py_proto PROTOS flip_spec.proto)

find_program(GRPC_PYTHON_PLUGIN grpc_python_plugin PATHS ENV PATH)
if(NOT GRPC_PYTHON_PLUGIN)
  set(GRPC_PYTHON_PLUGIN "/home/ubuntu/.conan2/p/b/grpc894ee3c31ed7e/p/bin/grpc_python_plugin")
endif()
protobuf_generate(
  TARGET flip_py_proto
  PROTOS flip_server.proto flip_spec.proto
  LANGUAGE grpc
  GENERATE_EXTENSIONS _pb2_grpc.py
  PLUGIN protoc-gen-grpc=${GRPC_PYTHON_PLUGIN})
