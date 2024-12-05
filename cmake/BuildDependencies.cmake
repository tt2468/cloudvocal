cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/build_conan)

if(WIN32)
  set(GRPC_VERSION v1.68.0)

  # get the prebuilt version from https://github.com/thommyho/Cpp-gRPC-Windows-PreBuilts/releases/
  include(FetchContent)

  set(grpc_url
      "https://github.com/thommyho/Cpp-gRPC-Windows-PreBuilts/releases/download/${GRPC_VERSION}/MSVC143_64.zip")
  FetchContent_Declare(grpc URL ${grpc_url} DOWNLOAD_EXTRACT_TIMESTAMP 1)
  FetchContent_MakeAvailable(grpc)

  find_package(OpenSSL CONFIG REQUIRED)

  # Specify include directories and link libraries for your project
  list(APPEND DEPS_INCLUDE_DIRS ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/include ${openssl_INCLUDE_DIRS_RELEASE})
  set(DEPS_LIB_DIRS ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/lib)
  set(PROTOC_EXECUTABLE ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin/protoc.exe)
  set(GRPC_PLUGIN_EXECUTABLE ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin/grpc_cpp_plugin.exe)

  # get all .lib files in the lib directory
  file(GLOB DEPS_LIBRARIES ${DEPS_LIB_DIRS}/*.lib)
else()
  find_package(gRPC CONFIG REQUIRED)
  find_package(protobuf CONFIG REQUIRED)
  find_package(absl CONFIG REQUIRED)
  find_package(ZLIB REQUIRED)
  find_package(OpenSSL CONFIG REQUIRED)
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
    DEPS_LIBRARIES
    ${grpc_LIBS_RELEASE}
    ${abseil_LIBS_RELEASE}
    ${protobuf_LIBS_RELEASE}
    ${ZLIB_LIBRARIES}
    ${openssl_LIBS_RELEASE}
    ${c-ares_LIBS_RELEASE}
    ${re2_LIBS_RELEASE})
  list(
    APPEND
    DEPS_LIB_DIRS
    ${grpc_LIB_DIRS_RELEASE}
    ${abseil_LIB_DIRS_RELEASE}
    ${protobuf_LIB_DIRS_RELEASE}
    ${zlib_LIB_DIRS_RELEASE}
    ${openssl_LIB_DIRS_RELEASE}
    ${c-ares_LIB_DIRS_RELEASE}
    ${re2_LIB_DIRS_RELEASE})
  list(
    APPEND
    DEPS_INCLUDE_DIRS
    ${gRPC_INCLUDE_DIRS}
    ${absl_INCLUDE_DIRS}
    ${protobuf_INCLUDE_DIRS_RELEASE}
    ${ZLIB_INCLUDE_DIRS}
    ${openssl_INCLUDE_DIRS_RELEASE}
    ${c-ares_INCLUDE_DIRS}
    ${re2_INCLUDE_DIRS})
endif()

message(STATUS "Dependencies include directories: ${DEPS_INCLUDE_DIRS}")
message(STATUS "Dependencies library directories: ${DEPS_LIB_DIRS}")
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
