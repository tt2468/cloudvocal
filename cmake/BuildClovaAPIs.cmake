set(PROTO_FILE ${CMAKE_SOURCE_DIR}/src/cloud-providers/clova/nest.proto)

message(STATUS "Generating C++ code from ${PROTO_FILE}")

# run protoc to generate the grpc files
add_custom_command(
  OUTPUT ${CMAKE_SOURCE_DIR}/src/cloud-providers/clova/nest.pb.cc
  COMMAND
    ${PROTOC_EXECUTABLE} --cpp_out=${CMAKE_SOURCE_DIR}/src/cloud-providers/clova
    --grpc_out=${CMAKE_SOURCE_DIR}/src/cloud-providers/clova --plugin=protoc-gen-grpc=${GRPC_PLUGIN_EXECUTABLE} -I
    ${CMAKE_SOURCE_DIR}/src/cloud-providers/clova ${PROTO_FILE}
  DEPENDS ${PROTO_FILE})
