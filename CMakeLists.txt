cmake_minimum_required(VERSION 3.2.1)
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_SOURCE_DIR}/bin")

project(model_viewer)
include_directories("${CMAKE_SOURCE_DIR}/include")
add_executable(${PROJECT_NAME} main.cpp)
find_package(raylib CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} raylib)