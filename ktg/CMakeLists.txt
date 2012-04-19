#
# OpenKTG CMake build rules
#
project(OpenKTG)

cmake_minimum_required(VERSION 2.8)

#
# libOpenKTG: texture generator library
set(openKTG_SOURCES gentexture.cpp)
set(openKTG_HEADERS gentexture.hpp types.hpp)

add_library(OpenKTG ${openKTG_SOURCES})

install(TARGETS OpenKTG ARCHIVE DESTINATION lib)
install(FILES ${openKTG_HEADERS} DESTINATION include/OpenKTG)

set(openKTG_DOC_FILES LICENSE README.txt)
install(FILES ${openKTG_DOC_FILES} DESTINATION share/doc/OpenKTG)

#
# demo executable
add_executable(demo demo.cpp)
target_link_libraries(demo OpenKTG)
