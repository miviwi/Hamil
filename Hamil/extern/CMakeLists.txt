set (ProjectDir "${PROJECT_SOURCE_DIR}/Hamil")

target_include_directories (Hamil PUBLIC
  "GL"
  "bullet3/include"
  "freetype2/include"
  "libyaml/include"
  "python37/include"
  "stb_image"
  "stb_rect_pack"
  "xxhash"
)

add_library (HamilExtern STATIC
  # gl3w
  "${ProjectDir}/extern/GL/gl3w.c"

  # stb_image
  "${ProjectDir}/extern/stb_image/image.c"

  # stb_rect_pack
  "${ProjectDir}/extern/stb_rect_pack/rectpack.c"
)
target_include_directories (HamilExtern PUBLIC
  "${ProjectDir}/extern"
)
target_link_libraries (HamilExtern PUBLIC
  # OpenGL/gl3w
  GL
  ${CMAKE_DL_LIBS}
)

# Copy over the correct ftoption.h depending on platform
if (WIN32)
  set (FTOPTION_H "${ProjectDir}/extern/freetype2/config/ftoption.win32.h")
endif ()

if (UNIX)
  set (FTOPTION_H "${ProjectDir}/extern/freetype2/config/ftoption.sysv.h")
endif ()

message ("-- Copying FreeType 2 configuration header (ftoption.h)")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy
  ${FTOPTION_H} "${ProjectDir}/extern/freetype2/include/freetype/config/ftoption.h"
)

# Copy over the correct pyconfig.h depending on platform
if (WIN32)
  set (PYCONFIG_H "${ProjectDir}/extern/python37/config/pyconfig.win32.h")
endif ()

if (UNIX)
  set (PYCONFIG_H "${ProjectDir}/extern/python37/config/pyconfig.sysv.h")
endif ()

message ("-- Copying Python configuration header (pyconfig.h)")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy
  ${PYCONFIG_H} "${ProjectDir}/extern/python37/include/pyconfig.h"
)
