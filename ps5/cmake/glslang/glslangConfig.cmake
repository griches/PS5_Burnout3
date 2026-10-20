# Burnout 3 for the PlayStation 5 - find_package(glslang CONFIG), answered by the
# glslang that ps5/cmake/ps5-game.cmake has already added to the build.
#
# SPDX-License-Identifier: MIT
if(NOT TARGET glslang)
	set(glslang_FOUND FALSE)
	return()
endif()
set(glslang_FOUND TRUE)
file(STRINGS ${GLSLANG_DIR}/CHANGES.md _glslang_release REGEX "^## [0-9]+\\.[0-9]+\\.[0-9]+" LIMIT_COUNT 1)
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" glslang_VERSION "${_glslang_release}")
foreach(target glslang glslang-default-resource-limits SPIRV)
	if(TARGET ${target} AND NOT TARGET glslang::${target})
		add_library(glslang::${target} ALIAS ${target})
	endif()
endforeach()
