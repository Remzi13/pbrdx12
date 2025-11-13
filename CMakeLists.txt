# Требуемая минимальная версия CMake
cmake_minimum_required(VERSION 3.10)

# Название нашего проекта и язык (C++)
project(rtdx12 CXX)

# Установим стандарт C++ (C++17 - хороший выбор)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("third_party/D3D12/include")
include_directories("third_party/Dxc/include")

set(SRC
    src/vector.h
    src/app.h
    src/app.cpp
    src/render.h
    src/render.cpp
    src/buffers.h
    src/buffers.cpp
    src/input.h
    src/input.cpp

    src/main.cpp
)
# Добавляем наш исполняемый файл, который будет собран из main.cpp
add_executable(rtdx12 ${SRC})

set_target_properties(rtdx12 PROPERTIES WIN32_EXECUTABLE TRUE)
target_compile_definitions(rtdx12 PRIVATE UNICODE _UNICODE)

target_link_libraries(rtdx12
        d3d12.lib
        dxgi.lib
        user32.lib
        dxcompiler.lib
)