cmake_minimum_required(VERSION 3.3)
project(ray-tracer)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin")
set(CXXFLAGS  "-Wall -Werror -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(
    rt
    src/main.cpp
    src/vec3.cpp
    src/obj.cpp
    src/cmd.cpp
)

target_compile_features(rt PUBLIC cxx_std_17)
target_link_libraries(
    rt
    pthread
)
