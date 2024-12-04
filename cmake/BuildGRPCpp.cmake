cmake_minimum_required(VERSION 3.14)

set(GRPC_VERSION v1.68.0)

if(WIN32)
  # get the prebuilt version from https://github.com/thommyho/Cpp-gRPC-Windows-PreBuilts/releases/
  include(FetchContent)

  set(grpc_url
      "https://github.com/thommyho/Cpp-gRPC-Windows-PreBuilts/releases/download/${GRPC_VERSION}/MSVC143_64.zip")
  FetchContent_Declare(grpc URL ${grpc_url} DOWNLOAD_EXTRACT_TIMESTAMP 1)
  FetchContent_MakeAvailable(grpc)

  # Specify include directories and link libraries for your project
  set(GRPC_INCLUDE_DIR ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/include)
  set(PROTOBUF_INCLUDE_DIR ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/include)
  set(GRPC_LIB_DIR ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/lib)
  set(PROTOC_EXECUTABLE ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin/protoc.exe)
  set(GRPC_PLUGIN_EXECUTABLE ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin/grpc_cpp_plugin.exe)

  # get all .lib files in the lib directory
  file(GLOB GRPC_LIBRARIES ${GRPC_LIB_DIR}/*.lib)
  set(GRPC_LIBRARIES
      ${GRPC_LIBRARIES}
      CACHE STRING "gRPC libraries")
else()
  # get grpc from conan
  list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/build_conan)
  find_package(gRPC CONFIG REQUIRED)
  find_package(protobuf CONFIG REQUIRED)
  find_package(absl CONFIG REQUIRED)
  find_package(ZLIB REQUIRED)
  find_package(OpenSSL REQUIRED)
  find_package(c-ares CONFIG REQUIRED)
  find_package(re2 CONFIG REQUIRED)

  set(PROTOC_EXECUTABLE
      ${Protobuf_PROTOC_EXECUTABLE}
      CACHE STRING "protoc executable")
  set(GRPC_PLUGIN_EXECUTABLE
      ${GRPC_CPP_PLUGIN_PROGRAM}
      CACHE STRING "gRPC plugin executable")
  list(
    APPEND
    GRPC_LIBRARIES
    ${grpc_LIBS_RELEASE}
    ${abseil_LIBS_RELEASE}
    ${protobuf_LIBS_RELEASE}
    ${ZLIB_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${c-ares_LIBS_RELEASE}
    ${re2_LIBS_RELEASE})
  list(
    APPEND
    GRPC_LIB_DIR
    ${grpc_LIB_DIRS_RELEASE}
    ${abseil_LIB_DIRS_RELEASE}
    ${protobuf_LIB_DIRS_RELEASE}
    ${zlib_LIB_DIRS_RELEASE}
    ${openssl_LIB_DIRS_RELEASE}
    ${c-ares_LIB_DIRS_RELEASE}
    ${re2_LIB_DIRS_RELEASE})
  set(GRPC_INCLUDE_DIR
      ${gRPC_INCLUDE_DIRS}
      CACHE STRING "gRPC include directory")
  set(PROTOBUF_INCLUDE_DIR
      ${protobuf_INCLUDE_DIRS}
      CACHE STRING "Protobuf include directory")
endif()

message(STATUS "gRPC include directory: ${GRPC_INCLUDE_DIR}")
message(STATUS "protobuf include directory: ${PROTOBUF_INCLUDE_DIR}")
message(STATUS "gRPC library directory: ${GRPC_LIB_DIR}")
message(STATUS "protoc executable: ${PROTOC_EXECUTABLE}")
message(STATUS "grpc_cpp_plugin executable: ${GRPC_PLUGIN_EXECUTABLE}")

if(NOT PROTOC_EXECUTABLE OR NOT GRPC_PLUGIN_EXECUTABLE)
  message(FATAL_ERROR "protoc or grpc_cpp_plugin not found")
endif()

# Add include directories
target_include_directories(
  ${CMAKE_PROJECT_NAME}
  PRIVATE ${GRPC_INCLUDE_DIR}
  PRIVATE ${PROTOBUF_INCLUDE_DIR})

# Link libraries
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${GRPC_LIB_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${GRPC_LIBRARIES})
