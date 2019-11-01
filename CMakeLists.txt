cmake_minimum_required (VERSION 3.0)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/Modules/")

project (Hamil)

add_subdirectory ("Eugene")
add_subdirectory ("Hamil")
