cmake_minimum_required(VERSION 3.3)
project(ray-tracer)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../bin")
set(CXXFLAGS  "-Wall -Werror -pedantic")
add_executable(
    rt
    src/main.cpp
    src/vec3.cpp
    src/obj.cpp
)
target_compile_features(rt PUBLIC cxx_std_17)
