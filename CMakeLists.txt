cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wpedantic")

# set the project name
project(qfuse2)

# add the executable
add_executable(qfuse2 src/main.cpp)
# target_compile_definitions(qfuse2 PRIVATE TRACE_LOGS)
target_compile_definitions(qfuse2 PRIVATE DEBUG_LOGS)
target_compile_definitions(qfuse2 PRIVATE INFO_LOGS)

include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        e69e5f977d458f2650bb346dadf2ad30c5320281) # 10.2.1
FetchContent_MakeAvailable(fmt)

target_link_libraries(qfuse2 PRIVATE fmt::fmt)

target_link_libraries(qfuse2 PRIVATE fuse3)

target_include_directories(qfuse2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    /usr/include/fuse3
)
