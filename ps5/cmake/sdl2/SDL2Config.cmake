# Burnout 3 for the PlayStation 5 - find_package(SDL2), answered by the SDL2
# that ps5/cmake/ps5-game.cmake has already added to the build (SDL2::SDL2).
#
# SPDX-License-Identifier: MIT
if(NOT TARGET SDL2::SDL2)
	set(SDL2_FOUND FALSE)
	return()
endif()
set(SDL2_FOUND TRUE)
set(SDL2_LIBRARIES SDL2::SDL2)
set(SDL2_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/SDL2/include ${CMAKE_BINARY_DIR}/SDL2/include/SDL2
	${CMAKE_BINARY_DIR}/SDL2/include-config-release)
set(SDL2_INCLUDE_DIR ${SDL2_INCLUDE_DIRS})
