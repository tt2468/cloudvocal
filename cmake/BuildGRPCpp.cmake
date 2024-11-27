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
  include(ExternalProject)

  # Set the gRPC version you want to use

  # Enable ccache if available
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    message(STATUS "ccache found: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
  endif()

  set(EXTRA_CMAKE_ARGS "")
  set(EXTRA_CMAKE_BUILD_ARGS "")

  # Define the external project for gRPC
  ExternalProject_Add(
    grpc
    PREFIX ${CMAKE_BINARY_DIR}/grpc
    GIT_REPOSITORY https://github.com/grpc/grpc.git
    GIT_TAG ${GRPC_VERSION}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    CMAKE_GENERATOR ${CMAKE_GENERATOR}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
               -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
               -DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF
               -DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF
               -DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF
               -DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF
               -DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF
               -DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF
               -DgRPC_INSTALL=ON
               -DgRPC_BUILD_TESTS=OFF
               ${EXTRA_CMAKE_ARGS}
    BUILD_COMMAND ${CMAKE_COMMAND} --build . --target install --config Release ${EXTRA_CMAKE_BUILD_ARGS}
    INSTALL_COMMAND ${CMAKE_COMMAND} --install <INSTALL_DIR> --config ${CMAKE_BUILD_TYPE}
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_INSTALL ON)

  # Specify include directories and link libraries for your project
  ExternalProject_Get_Property(grpc install_dir)
  set(GRPC_INCLUDE_DIR ${install_dir}/include)
  set(GRPC_LIB_DIR ${install_dir}/lib)
  set(PROTOC_EXECUTABLE ${install_dir}/bin/protoc)
  set(GRPC_PLUGIN_EXECUTABLE ${install_dir}/bin/grpc_cpp_plugin)

  # get all .a files in the lib directory
  file(GLOB GRPC_LIBRARIES ${GRPC_LIB_DIR}/*.a)
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
