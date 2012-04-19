#
# CMake find_package macro for libOpenKTG
#
# This file can be included in projects using libOpenKTG to automatically
# locate the library & include directories.
#

if(QIRC_INCLUDE_DIR)
  set(OpenKTG_FIND_QUIETLY TRUE)
endif(OpenKTG_INCLUDE_DIR)

find_path(OpenKTG_INCLUDE_DIR OpenKTG/gentexture.hpp
  PATHS
  "$ENV{OpenKTG}/include"
  /usr/local/include
  /usr/pkg/include
  /usr/include
)

find_library(OpenKTG_LIBRARIES OpenKTG
  PATHS
  "$ENV{OpenKTG}/lib"
  /usr/local/lib
  /usr/pkg/lib
  /usr/lib
)

set(OpenKTG_FOUND FALSE)
if(OpenKTG_INCLUDE_DIR)
  if(OpenKTG_LIBRARIES)
    set(OpenKTG_FOUND TRUE)
    if(NOT OpenKTG_FIND_QUIETLY)
      message(STATUS "Found libOpenKTG: ${OpenKTG_LIBRARIES}")
    endif(NOT OpenKTG_FIND_QUIETLY)
  endif(OpenKTG_LIBRARIES)
endif(OpenKTG_INCLUDE_DIR)

mark_as_advanced(
  OpenKTG_INCLUDE_DIR
  OpenKTG_LIBRARIES
)