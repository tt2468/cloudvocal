set(CLOVA_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/cloud-providers/clova)
set(PROTO_FILE ${CLOVA_SOURCE_DIR}/nest.proto)
set(CLOVA_OUTPUT_DIR ${CLOVA_SOURCE_DIR})

message(STATUS "Generating C++ code from ${PROTO_FILE}")

# run protoc to generate the grpc files
add_custom_command(
  OUTPUT ${CLOVA_SOURCE_DIR}/nest.pb.cc ${CLOVA_SOURCE_DIR}/nest.pb.h ${CLOVA_SOURCE_DIR}/nest.grpc.pb.cc
         ${CLOVA_SOURCE_DIR}/nest.grpc.pb.h
  COMMAND ${PROTOC_EXECUTABLE} --cpp_out=${CLOVA_OUTPUT_DIR} --grpc_out=${CLOVA_OUTPUT_DIR}
          --plugin=protoc-gen-grpc=${GRPC_PLUGIN_EXECUTABLE} -I ${CLOVA_OUTPUT_DIR} ${PROTO_FILE}
  DEPENDS ${PROTO_FILE})

add_library(clova-apis ${CLOVA_SOURCE_DIR}/nest.pb.cc ${CLOVA_SOURCE_DIR}/nest.grpc.pb.cc)

# disable conversion warnings from the generated files
if(MSVC)
  target_compile_options(clova-apis PRIVATE /wd4244 /wd4267)
else()
  target_compile_options(
    clova-apis
    PRIVATE -fPIC
            -Wno-conversion
            -Wno-sign-conversion
            -Wno-unused-parameter
            -Wno-unused-variable
            -Wno-error=shadow
            -Wno-shadow
            -Wno-error=conversion)
endif()

# Add include directories
target_include_directories(clova-apis PUBLIC ${CLOVA_OUTPUT_DIR} ${GRPC_INCLUDE_DIR} ${PROTOBUF_INCLUDE_DIR}
                                             ${absl_INCLUDE_DIRS})

# link the library to the main project
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE clova-apis)
