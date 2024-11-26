cmake_minimum_required(VERSION 3.11)

include(FetchContent)

# Fetch the googleapis repository
FetchContent_Declare(
  googleapis
  GIT_REPOSITORY https://github.com/googleapis/googleapis.git
  GIT_TAG master)

FetchContent_MakeAvailable(googleapis)

# Define the proto file
set(PROTO_FILES
    ${googleapis_SOURCE_DIR}/google/cloud/speech/v1/cloud_speech.proto
    ${googleapis_SOURCE_DIR}/google/cloud/speech/v1/resource.proto
    ${googleapis_SOURCE_DIR}/google/api/annotations.proto
    ${googleapis_SOURCE_DIR}/google/api/http.proto
    ${googleapis_SOURCE_DIR}/google/api/client.proto
    ${googleapis_SOURCE_DIR}/google/api/launch_stage.proto
    ${googleapis_SOURCE_DIR}/google/api/field_behavior.proto
    ${googleapis_SOURCE_DIR}/google/api/resource.proto
    ${googleapis_SOURCE_DIR}/google/longrunning/operations.proto
    ${googleapis_SOURCE_DIR}/google/rpc/status.proto)

set(OUTPUT_FOLDER ${CMAKE_SOURCE_DIR}/src/cloud-providers/google)

set(GOOGLE_APIS_OUTPUT_FILES "")

# run protoc to generate the grpc files
foreach(PROTO_FILE ${PROTO_FILES})
  message(STATUS "Generating C++ code from ${PROTO_FILE}")

  # create .pb.cc and .pb.h file paths from the proto file
  get_filename_component(PROTO_FILE_NAME ${PROTO_FILE} NAME)
  string(REPLACE ".proto" ".pb.cc" PROTO_CC_FILE ${PROTO_FILE_NAME})
  string(REPLACE ".proto" ".pb.h" PROTO_H_FILE ${PROTO_FILE_NAME})
  string(REPLACE ".proto" ".grpc.pb.cc" PROTO_GRPC_FILE ${PROTO_FILE_NAME})
  string(REPLACE ".proto" ".grpc.pb.h" PROTO_GRPC_H_FILE ${PROTO_FILE_NAME})

  # get proto file path relative to the googleapis directory
  file(RELATIVE_PATH PROTO_FILE_RELATIVE ${googleapis_SOURCE_DIR} ${PROTO_FILE})
  # get the directory of the proto file
  get_filename_component(PROTO_FILE_DIR ${PROTO_FILE_RELATIVE} DIRECTORY)

  # append the output files to the list
  list(APPEND GOOGLE_APIS_OUTPUT_FILES ${OUTPUT_FOLDER}/${PROTO_FILE_DIR}/${PROTO_CC_FILE}
       ${OUTPUT_FOLDER}/${PROTO_FILE_DIR}/${PROTO_GRPC_FILE})

  # Generate the grpc files
  add_custom_command(
    OUTPUT ${OUTPUT_FOLDER}/${PROTO_FILE_DIR}/${PROTO_CC_FILE} ${OUTPUT_FOLDER}/${PROTO_FILE_DIR}/${PROTO_GRPC_FILE}
    COMMAND ${PROTOC_EXECUTABLE} --cpp_out=${OUTPUT_FOLDER} --grpc_out=${OUTPUT_FOLDER}
            --plugin=protoc-gen-grpc=${GRPC_PLUGIN_EXECUTABLE} -I ${googleapis_SOURCE_DIR} ${PROTO_FILE}
    DEPENDS ${PROTO_FILE})
endforeach()

# add a library for the generated files
add_library(google-apis ${GOOGLE_APIS_OUTPUT_FILES})

# disable conversion warnings from the generated files
if(MSVC)
  target_compile_options(google-apis PRIVATE /wd4244 /wd4267)
else()
  target_compile_options(google-apis PRIVATE -Wno-conversion)
endif()

target_include_directories(google-apis PUBLIC ${OUTPUT_FOLDER} ${GRPC_INCLUDE_DIR})

# link the library to the main project
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE google-apis)
