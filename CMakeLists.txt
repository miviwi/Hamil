cmake_minimum_required (VERSION 3.0)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/Modules/")

option (DUMP_COMPILE_COMMANDS "Enable dumping 'compile_commands.json' into Hamil/")

project (Hamil)

add_subdirectory ("Eugene")
add_subdirectory ("Hamil")
