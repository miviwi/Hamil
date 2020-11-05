if (WIN32)
  set (Platform "win32")
endif ()

if (UNIX)
  set (Platform "sysv")
endif ()

set (CMakeScriptsDir "${PROJECT_SOURCE_DIR}/CMake/Scripts")

include ("${CMakeScriptsDir}/${Platform}/FindPackages.cmake")

set (SolutionDir "${PROJECT_SOURCE_DIR}")
set (ProjectDir  "${PROJECT_SOURCE_DIR}/Hamil")
set (OutDir      "${PROJECT_BINARY_DIR}")

set (Generated   "${ProjectDir}/Generated")

#     --- Configure headers ---
# config.h.in configuration header variables
set (Hamil__PLATFORM    "\"${Platform}\"")
if (WIN32)
  set (Hamil__win32   1)
  set (Hamil__sysv    0)
endif()
if (UNIX)
  set (Hamil__win32   0)
  set (Hamil__sysv    1)
endif()

set (Hamil__PROJECT_VERSION       0x001000)
set (Hamil__PROJECT_VERSION_STR   "\"0.10.0\"")

set (Hamil__PROJECT_DIR "R\"(${OutDir})\"")

message ("-- Configuring header files")

message ("-- Configuring config.in.h -> config")
configure_file (
  "${ProjectDir}/config/config.h.in" "${Generated}/config"
)

#     --- Eugene location/build script ---
set (EUGENE "${SolutionDir}/Eugene/eugene")
set (EUGENE_BUILD "${ProjectDir}/build.eugene")

#     --- Create the 'Hamil' target ---
if (WIN32)
  set (HAMIL_EXECUTABLE_MAIN
    "${ProjectDir}/src/winmain.cpp")
endif()
if (UNIX)
  set (HAMIL_EXECUTABLE_MAIN
    "${ProjectDir}/src/sysvmain.cpp")
endif()

add_executable(Hamil
  # main()
  ${HAMIL_EXECUTABLE_MAIN}
)

#     --- Project executable compilation setup ---
add_subdirectory ("src")
add_subdirectory ("extern")

set (CMAKE_CXX_COMPILER g++)
target_compile_options (Hamil PUBLIC
  -pipe

  -std=c++17

  -ffast-math
  -mmovbe -msse -msse2 -msse3 -mssse3 -msse4 -mf16c

  -Wall -Wno-switch -Wno-unknown-pragmas -Wno-unused-function
  -Wno-writable-strings     # clang
  -Wno-write-strings        # gcc
  -Wno-address-of-packed-member

  -g
)

#target_compile_definitions (Hamil PUBLIC
#  NDEBUG
#)

target_include_directories (Hamil PUBLIC
  "include"
  "extern"

  "Generated"

  # libevdev
  ${LIBEVDEV_INCLUDE_DIRS}
)

target_link_libraries (Hamil PRIVATE
  HamilExtern

  stdc++    # needed for clang (?)

  # math
  m

  # pthreads
  ${CMAKE_THREAD_LIBS_INIT}

  # FreeType 2
  ${FREETYPE_LIBRARIES}

  # libyaml
  ${LIBYAML_LIBRARIES}

  # Bullet physics
  ${BULLET_LIBRARIES}

  # Python 3.7+
  ${PYTHON_LIBRARIES}

  # X11/xcb
  ${X11_XCB_LIBRARIES}

  # libevdev
  ${LIBEVDEV_LIBRARIES}
)

target_sources (Hamil PUBLIC
  # OpenGL shader uniform interface structs
  "${ProjectDir}/src/programs.cpp"
)

#     --- Setup running eugene resource,uniform,component header generation as a build step ---
set (EugeneWorkingDir "${OutDir}/.eugene")
set (ResourceDir "${OutDir}/res")

# Ensure the 'EugeneWorkingDir' exists (i.e. create it if it doesn't)
if (NOT EXISTS ${EugeneWorkingDir})
  message ("-- Creating eugene working (build/.eugene/) and input (build/res/) directories")
  execute_process (COMMAND ${CMAKE_COMMAND} -E make_directory ${EugeneWorkingDir} ${ResourceDir})
endif ()

set (EugeneVars "Solution=${SolutionDir}/" "Generated=${Generated}" "Resources=${ResourceDir}" "Output=${OutDir}")
add_custom_target (RunEugene
  COMMAND
    ${EUGENE} run ${EUGENE_BUILD} ${EugeneVars}
  BYPRODUCTS
    "${Generated}/uniforms.cpp" "${Generated}/resources.cpp" "${Generated}/components.cpp"
  WORKING_DIRECTORY
    ${EugeneWorkingDir}
  COMMENT
    "Run Eugene")

add_dependencies (RunEugene Eugene)
add_dependencies (Hamil RunEugene)

# Add eugene output *.cpp files to the sources
target_sources (Hamil PRIVATE
  "${Generated}/uniforms.cpp"
  "${Generated}/resources.cpp"
  "${Generated}/components.cpp"
)

#     --- Copy all required runtime dynamic libraries ---
if (WIN32)
  # FreeType
  add_custom_command (TARGET Hamil POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different "${ProjectDir}/extern/freetype2/win64/freetype271MT.dll" ${OutDir}
    COMMENT
      "Copy freetype2 runtime lib")
endif()

#     --- YouCompleteMe compilation flag database ---
add_custom_command (TARGET Hamil POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_BINARY_DIR}/../compile_commands.json" "${ProjectDir}/compile_commands.json"
  COMMENT
    "-- Copying compile flags dump for YouCompleteMe (compile_commands.json)"
)
