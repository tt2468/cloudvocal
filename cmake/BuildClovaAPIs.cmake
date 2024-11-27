set(PROTO_FILE ${CMAKE_SOURCE_DIR}/src/cloud-providers/clova/nest.proto)
set(CLOVA_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/src/cloud-providers/clova)

message(STATUS "Generating C++ code from ${PROTO_FILE}")

# run protoc to generate the grpc files
add_custom_command(
  OUTPUT ${CMAKE_SOURCE_DIR}/src/cloud-providers/clova/nest.pb.cc
         ${CMAKE_SOURCE_DIR}/src/cloud-providers/clova/nest.pb.h
         ${CMAKE_SOURCE_DIR}/src/cloud-providers/clova/nest.grpc.pb.cc
         ${CMAKE_SOURCE_DIR}/src/cloud-providers/clova/nest.grpc.pb.h
  COMMAND ${PROTOC_EXECUTABLE} --cpp_out=${CLOVA_OUTPUT_DIR} --grpc_out=${CLOVA_OUTPUT_DIR}
          --plugin=protoc-gen-grpc=${GRPC_PLUGIN_EXECUTABLE} -I ${CLOVA_OUTPUT_DIR} ${PROTO_FILE}
  DEPENDS ${PROTO_FILE})
