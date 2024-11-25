cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
list(APPEND CMAKE_PREFIX_PATH "d:/Dmitriy/ITMO/CompGraphics/mse-gl-hw-2/thirdparty/draco")
list(APPEND CMAKE_PREFIX_PATH "d:/Dmitriy/ITMO/CompGraphics/mse-gl-hw-2/thirdparty/draco/cmake")

project(mse-gl-hw-template VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(thirdparty)

include_directories(src)

# For Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_subdirectory(src/Base)
add_subdirectory(src/App)
