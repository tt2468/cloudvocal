cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/build_conan)

find_package(gRPC CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)
find_package(c-ares CONFIG REQUIRED)
find_package(re2 CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED)
find_package(AWSSDK CONFIG REQUIRED)
find_package(BZip2 CONFIG REQUIRED)

set(PROTOC_EXECUTABLE
    ${Protobuf_PROTOC_EXECUTABLE}
    CACHE STRING "protoc executable")
set(GRPC_PLUGIN_EXECUTABLE
    ${GRPC_CPP_PLUGIN_PROGRAM}
    CACHE STRING "gRPC plugin executable")
list(
  APPEND
  DEPS_LIBRARIES
  ${grpc_LIBS_RELEASE}
  ${abseil_LIBS_RELEASE}
  ${protobuf_LIBS_RELEASE}
  ${openssl_LIBS_RELEASE}
  openssl::openssl
  ${c-ares_LIBS_RELEASE}
  ${re2_LIBS_RELEASE}
  ${boost_Boost_url_LIBS_RELEASE}
  ${aws-c-auth_LIBS_RELEASE}
  ${aws-c-cal_LIBS_RELEASE}
  ${aws-c-common_LIBS_RELEASE}
  ${aws-c-compression_LIBS_RELEASE}
  ${aws-c-event-stream_LIBS_RELEASE}
  ${aws-c-http_LIBS_RELEASE}
  ${aws-c-io_LIBS_RELEASE}
  ${aws-c-mqtt_LIBS_RELEASE}
  ${aws-c-s3_LIBS_RELEASE}
  ${aws-c-sdkutils_LIBS_RELEASE}
  ${aws-checksums_LIBS_RELEASE}
  ${aws-crt-cpp_LIBS_RELEASE}
  ${aws-sdk-cpp_LIBS_RELEASE}
  ${bzip2_LIBS_RELEASE})
if(WIN32)
  list(
    APPEND
    DEPS_LIBRARIES
    userenv
    advapi32
    ws2_32
    crypt32
    bcrypt
    winhttp
    shlwapi
    wininet
    secur32
    iphlpapi
    netapi32
    rpcrt4
    shell32
    version
    ncrypt)
else()
  list(APPEND DEPS_LIBRARIES ${ZLIB_LIBRARIES})
endif()
if(APPLE)
  list(APPEND DEPS_LIBRARIES resolv)
endif()

list(
  APPEND
  DEPS_LIB_DIRS
  ${grpc_LIB_DIRS_RELEASE}
  ${abseil_LIB_DIRS_RELEASE}
  ${protobuf_LIB_DIRS_RELEASE}
  ${zlib_LIB_DIRS_RELEASE}
  ${openssl_LIB_DIRS_RELEASE}
  ${c-ares_LIB_DIRS_RELEASE}
  ${re2_LIB_DIRS_RELEASE}
  ${boost_LIB_DIRS_RELEASE}
  ${aws-c-auth_LIB_DIRS_RELEASE}
  ${aws-c-cal_LIB_DIRS_RELEASE}
  ${aws-c-common_LIB_DIRS_RELEASE}
  ${aws-c-compression_LIB_DIRS_RELEASE}
  ${aws-c-event-stream_LIB_DIRS_RELEASE}
  ${aws-c-http_LIB_DIRS_RELEASE}
  ${aws-c-io_LIB_DIRS_RELEASE}
  ${aws-c-mqtt_LIB_DIRS_RELEASE}
  ${aws-c-sdkutils_LIB_DIRS_RELEASE}
  ${aws-c-s3_LIB_DIRS_RELEASE}
  ${aws-checksums_LIB_DIRS_RELEASE}
  ${aws-crt-cpp_LIB_DIRS_RELEASE}
  ${aws-sdk-cpp_LIB_DIRS_RELEASE}
  ${bzip2_LIB_DIRS_RELEASE})
list(
  APPEND
  DEPS_INCLUDE_DIRS
  ${Boost_INCLUDE_DIRS}
  ${gRPC_INCLUDE_DIRS}
  ${absl_INCLUDE_DIRS}
  ${protobuf_INCLUDE_DIRS_RELEASE}
  ${ZLIB_INCLUDE_DIRS}
  ${openssl_INCLUDE_DIRS_RELEASE}
  ${c-ares_INCLUDE_DIRS}
  ${re2_INCLUDE_DIRS}
  ${aws-c-auth_INCLUDE_DIRS_RELEASE}
  ${aws-c-cal_INCLUDE_DIRS_RELEASE}
  ${aws-c-common_INCLUDE_DIRS_RELEASE}
  ${aws-c-compression_INCLUDE_DIRS_RELEASE}
  ${aws-c-event-stream_INCLUDE_DIRS_RELEASE}
  ${aws-c-http_INCLUDE_DIRS_RELEASE}
  ${aws-c-io_INCLUDE_DIRS_RELEASE}
  ${aws-c-mqtt_INCLUDE_DIRS_RELEASE}
  ${aws-c-sdkutils_INCLUDE_DIRS_RELEASE}
  ${aws-c-s3_INCLUDE_DIRS_RELEASE}
  ${aws-checksums_INCLUDE_DIRS_RELEASE}
  ${aws-crt-cpp_INCLUDE_DIRS_RELEASE}
  ${aws-sdk-cpp_INCLUDE_DIRS_RELEASE}
  ${bzip2_INCLUDE_DIRS_RELEASE})

message(STATUS "Dependencies include directories: ${DEPS_INCLUDE_DIRS}")
message(STATUS "Dependencies library directories: ${DEPS_LIB_DIRS}")
message(STATUS "Dependencies libraries: ${DEPS_LIBRARIES}")
message(STATUS "protoc executable: ${PROTOC_EXECUTABLE}")
message(STATUS "grpc_cpp_plugin executable: ${GRPC_PLUGIN_EXECUTABLE}")

if(NOT PROTOC_EXECUTABLE OR NOT GRPC_PLUGIN_EXECUTABLE)
  message(FATAL_ERROR "protoc or grpc_cpp_plugin not found")
endif()

# Add include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${DEPS_INCLUDE_DIRS})

# Link libraries
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${DEPS_LIB_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${DEPS_LIBRARIES})
