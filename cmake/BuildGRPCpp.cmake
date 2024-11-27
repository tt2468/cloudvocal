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
  set(GRPC_LIB_DIR ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/lib)
  set(PROTOC_EXECUTABLE ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin/protoc.exe)
  set(GRPC_PLUGIN_EXECUTABLE ${grpc_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin/grpc_cpp_plugin.exe)

  # get all .lib files in the lib directory
  file(GLOB GRPC_LIBRARIES ${GRPC_LIB_DIR}/*.lib)
  set(GRPC_LIBRARIES
      ${GRPC_LIBRARIES}
      CACHE STRING "gRPC libraries")
else()
  # ensure the grpc enviroment variables (GRPC_SOURCE_DIR, GRPC_BUILD_DIR) are set
  if(NOT DEFINED ENV{GRPC_SOURCE_DIR} OR NOT DEFINED ENV{GRPC_BUILD_DIR})
    message(FATAL_ERROR "GRPC_SOURCE_DIR and GRPC_BUILD_DIR environment variables not set")
  endif()

  # make sure the protoc and grpc_cpp_plugin are found
  find_program(
    PROTOC_EXECUTABLE
    NAMES protoc
    PATHS $ENV{GRPC_BUILD_DIR}/external/com_google_protobuf/)
  find_program(
    GRPC_PLUGIN_EXECUTABLE
    NAMES grpc_cpp_plugin
    PATHS $ENV{GRPC_BUILD_DIR}/src/compiler/)

  if(NOT PROTOC_EXECUTABLE OR NOT GRPC_PLUGIN_EXECUTABLE)
    message(STATUS "protoc: ${PROTOC_EXECUTABLE}")
    message(STATUS "grpc_cpp_plugin: ${GRPC_PLUGIN_EXECUTABLE}")
    message(FATAL_ERROR "protoc or grpc_cpp_plugin not found")
  endif()

  # make sure the grpc include dir is found
  set(GRPC_INCLUDE_DIR $ENV{GRPC_SOURCE_DIR})
  if(NOT EXISTS ${GRPC_INCLUDE_DIR})
    message(FATAL_ERROR "gRPC include directory not found")
  endif()

  # get all .a files in the lib directory
  file(GLOB GRPC_LIBRARIES "$ENV{GRPC_BUILD_DIR}/*.a")
  set(GRPC_LIBRARIES
      ${GRPC_LIBRARIES}
      CACHE STRING "gRPC libraries")
endif()

message(STATUS "gRPC include directory: ${GRPC_INCLUDE_DIR}")
message(STATUS "gRPC library directory: ${GRPC_LIB_DIR}")
message(STATUS "protoc executable: ${PROTOC_EXECUTABLE}")
message(STATUS "grpc_cpp_plugin executable: ${GRPC_PLUGIN_EXECUTABLE}")

# Add include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${GRPC_INCLUDE_DIR})

# Link libraries
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${GRPC_LIB_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${GRPC_LIBRARIES})
