cmake_minimum_required(VERSION 3.20)
project(lol_range_overlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME}
    PRIVATE
        src/main.cpp
        src/memory_scanner.cpp
        src/dx_hook.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
)

find_package(DirectX REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        d3d11
        dxgi
)

