add_executable (Hamil)

set (SolutionDir "${PROJECT_SOURCE_DIR}")
set (ProjectDir  "${PROJECT_SOURCE_DIR}/Hamil")
set (OutDir      "${PROJECT_BINARY_DIR}")

set (Generated   "${ProjectDir}/Generated")

if (WIN32)
  set (EUGENE "${SolutionDir}/Eugene/src/eugene")
endif ()

if (UNIX)
  set (EUGENE "${SolutionDir}/Eugene/eugene")
endif ()

set (EUGENE_BUILD "${ProjectDir}/build.eugene")

add_subdirectory ("src")
add_subdirectory ("extern")

target_compile_options (Hamil PUBLIC -std=c++17 -ffast-math -msse -msse2 -msse4 -mf16c)
target_include_directories (Hamil PUBLIC
  "include"
  "Generated"
)

target_sources (Hamil PUBLIC
  "${ProjectDir}/src/winmain.cpp"
  "${ProjectDir}/src/programs.cpp")

add_custom_target (RunEugene
  COMMAND
    ${EUGENE} run ${EUGENE_BUILD} "Solution=${SolutionDir}" "Generated=${Generated}" "Output=${OutDir}"
  BYPRODUCTS
    "${Generated}/uniforms.cpp" "${Generated}/resources.cpp" "${Generated}/components.cpp"
  COMMENT
    "Run Eugene")

add_dependencies (RunEugene Eugene)
add_dependencies (Hamil RunEugene)

add_custom_command (TARGET Hamil POST_BUILD
  COMMAND
    cp "${ProjectDir}/extern/freetype2/win64/freetype271MT.dll" ${OutDir}
  COMMENT
    "Copy freetype2 runtime lib")

if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
  execute_process (COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json)
endif ()
