cmake_minimum_required(VERSION 3.13)
project(tinyrenderer)

set(CMAKE_CXX_STANDARD 14)

include_directories(${CMAKE_CURRENT_LIST_DIR})

set(SRC_CORE
        geometry.cpp
        model.cpp
        gl.cpp
        mat.cpp
        tgaimage.cpp)
add_executable(tinyrenderer ${SRC_CORE} main.cpp)