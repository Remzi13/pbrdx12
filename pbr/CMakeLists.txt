cmake_minimum_required(VERSION 3.10)

project(pbr CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC
    ../src/vector.h
    ../src/scene.h
    ../src/scene.cpp
    ../src/utils.h
    ../src/utils.cpp
    main.cpp
)
add_executable(pbr ${SRC})

