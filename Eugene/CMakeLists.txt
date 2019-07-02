add_custom_target (Eugene)

set (SolutionDir "${PROJECT_SOURCE_DIR}")
set (ProjectDir  "${PROJECT_SOURCE_DIR}/Eugene")
set (OutDir      "${PROJECT_BINARY_DIR}")

if (WIN32)
  set (BUILD_EUGENE "${ProjectDir}/build")
endif(WIN32)

if (UNIX)
  set (BUILD_EUGENE "${ProjectDir}/build.sh")
endif(UNIX)

add_custom_command (TARGET Eugene PRE_BUILD
  COMMAND
    ${BUILD_EUGENE}
  DEPENDS
    "${ProjectDir}/src/componentgen.py"
    "${ProjectDir}/src/cxx.py"
    "${ProjectDir}/src/database.py"
    "${ProjectDir}/src/eugene_modules.py"
    "${ProjectDir}/src/eugene.py"
    "${ProjectDir}/src/eugene_script.py"
    "${ProjectDir}/src/eugene_util.py"
    "${ProjectDir}/src/resourcegen.py"
    "${ProjectDir}/src/uniformgen.py"

    "${ProjectDir}/win32/eugene_win32.c"
    "${ProjectDir}/win32/eugene_sysv.c"
  COMMENT
    "Build eugene_win32/eugene_sysv module")
