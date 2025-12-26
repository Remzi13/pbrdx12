# Требуемая минимальная версия CMake
cmake_minimum_required(VERSION 3.10)

# Название нашего проекта и язык (C++)
project(prtdx12 CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("third_party/D3D12/include")
include_directories("third_party/Dxc/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

set(SRC
    src/vector.h
    src/app.h
    src/app.cpp
    src/input.h
    src/input.cpp
    src/scene.h
    src/scene.cpp
    src/utils.h
    src/utils.cpp

    src/main.cpp
)

add_subdirectory("src/render")
add_subdirectory("src/core")

add_executable(pbrdx12 ${SRC})

set_target_properties(pbrdx12 PROPERTIES WIN32_EXECUTABLE TRUE)
target_compile_definitions(pbrdx12 PRIVATE UNICODE _UNICODE)

target_link_libraries(pbrdx12 debug render core)

target_link_libraries(pbrdx12
    d3d12.lib
    dxgi.lib
    user32.lib
    dxcompiler.lib
)