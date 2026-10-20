# Burnout 3 for the PlayStation 5 - what the console adds to Takedawn's own build.
#
# tools/build.sh configures vendor/takedawn with the payload SDK's toolchain and
# names this file as CMAKE_PROJECT_burnout3_recomp_INCLUDE: CMake includes it
# right after the project() call, before Takedawn and its toolkit read their
# options and look for their dependencies. It provides:
#
#   SDL2     .deps/SDL2, patched (tools/setup-sdl.sh), with the drivers in ps5/sdl
#   glslang  .deps/glslang (tools/setup-glslang.sh), static, without the optimiser:
#            the toolkit compiles the NV2A's shaders to SPIR-V at run time
#   Vulkan   the headers of PS5_Vulkan's RADV archive; no loader: every command
#            comes from RADV's entry point (the toolkit's pointer table, android_vk.h)
#   options  the console's choices among Takedawn's own
#
# and makes the executable's link PS5_Vulkan's (tools/link-title.sh, with RADV).
#
# SPDX-License-Identifier: MIT

foreach(required PS5_PORT_DIR PS5_VULKAN_DIR SDL_DIR GLSLANG_DIR)
	if(NOT ${required})
		message(FATAL_ERROR "ps5-game.cmake: ${required} is not set (use ps5/tools/build.sh)")
	endif()
endforeach()
set(ps5 ${PS5_PORT_DIR}/ps5)

# -fno-omit-frame-pointer: the console's crash backtraces are frame-pointer walks.
add_compile_options(-g1 -ffunction-sections -fdata-sections -fno-omit-frame-pointer)
# The console's CPU is a Zen 2: the toolchain's default is the first x86-64.
add_compile_options(-march=znver2)
# An init array, in the order the source has: tools/cc-init-array.sh has the reason
set(CMAKE_C_COMPILER_LAUNCHER sh ${ps5}/tools/cc-init-array.sh)
set(CMAKE_CXX_COMPILER_LAUNCHER sh ${ps5}/tools/cc-init-array.sh)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
# Headers a Linux program expects that the console's C library has elsewhere (ps5/include)
include_directories(BEFORE ${ps5}/include)

# The platform archive, by its file and not its target: it is linked whole into the title (below)
set(ps5_platform_archive ${CMAKE_BINARY_DIR}/libps5_platform.a)

# The console: klog, the pad, and the libc names a title cannot import
add_library(ps5_platform STATIC ${ps5}/src/platform.c ${ps5}/src/shims.c ${ps5}/src/startup.c ${ps5}/src/paths.c ${ps5}/src/codecopy.c
	${ps5}/src/devloop_shot.c)
target_include_directories(ps5_platform PUBLIC ${ps5}/src)
file(READ ${ps5}/sce_sys/param.json param_json)
string(JSON PS5_TITLE_NAME GET "${param_json}" localizedParameters en-US titleName)
target_compile_definitions(ps5_platform PRIVATE "PS5_TITLE_NAME=\"${PS5_TITLE_NAME}\"")
set_target_properties(ps5_platform PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# zlib is in the RADV archive already (Mesa's own): its headers, for devloop_shot.c's PNGs
target_include_directories(ps5_platform PRIVATE ${PS5_VULKAN_DIR}/.deps/native/zlib/root/usr/include)

# SDL2: threads, time and files as a FreeBSD has them; video, input and audio from ps5/sdl
set(SDL_PS5_PORT_DIR ${ps5}/sdl CACHE INTERNAL "")
set(SDL_PS5_PLATFORM_DIR ${ps5}/src CACHE INTERNAL "")
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_SDL2MAIN ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
foreach(off SDL_SHARED SDL_TEST SDL_TESTS
		SDL_X11 SDL_WAYLAND SDL_KMSDRM SDL_RPI SDL_VIVANTE SDL_OFFSCREEN SDL_DUMMYVIDEO SDL_DIRECTFB
		SDL_OPENGL SDL_OPENGLES SDL_VULKAN
		SDL_OSS SDL_ALSA SDL_JACK SDL_ESD SDL_ARTS SDL_NAS SDL_PIPEWIRE SDL_PULSEAUDIO SDL_SNDIO SDL_FUSIONSOUND
		SDL_DISKAUDIO SDL_DUMMYAUDIO SDL_LIBSAMPLERATE
		SDL_DBUS SDL_IBUS SDL_LIBUDEV SDL_HIDAPI SDL_HIDAPI_LIBUSB SDL_HIDAPI_JOYSTICK SDL_VIRTUAL_JOYSTICK
		SDL_SYSTEM_ICONV SDL_LIBICONV SDL_RPATH SDL_SENSOR SDL_HAPTIC SDL_POWER)
	set(${off} OFF CACHE BOOL "" FORCE)
endforeach()
# (SDL_LOADSO stays on: without it SDL drops its Vulkan support. SDL_VULKAN is SDL's switch for a
# desktop's loader: ps5/patches/sdl.patch turns Vulkan on for the console itself)
add_subdirectory(${SDL_DIR} ${CMAKE_BINARY_DIR}/SDL2 EXCLUDE_FROM_ALL)
target_include_directories(ps5_platform PRIVATE ${SDL_DIR}/include)
if(NOT TARGET SDL2::SDL2)
	add_library(SDL2::SDL2 ALIAS SDL2-static)
endif()
# <SDL.h> and <SDL2/SDL.h> both: the build tree has its headers under include/SDL2
set(SDL2_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/SDL2/include ${CMAKE_BINARY_DIR}/SDL2/include/SDL2
	${CMAKE_BINARY_DIR}/SDL2/include-config-release)
include_directories(${SDL2_INCLUDE_DIRS})
# find_package(SDL2) in the toolkit: the target above (ps5/cmake/sdl2/SDL2Config.cmake)
set(SDL2_DIR ${ps5}/cmake/sdl2 CACHE PATH "" FORCE)

# Vulkan: RADV's headers. find_package(Vulkan) wants a library too: the platform
# archive stands in (the driver itself is linked by tools/link-title.sh).
set(ps5_vulkan_headers ${PS5_VULKAN_DIR}/.deps/native/radv-release/include)
set(Vulkan_INCLUDE_DIR ${ps5_vulkan_headers} CACHE PATH "" FORCE)
set(Vulkan_LIBRARY ${ps5_platform_archive} CACHE FILEPATH "" FORCE)

# glslang: its C interface and default resource limits, static, no SPIRV-Tools
enable_language(CXX)
foreach(off ENABLE_OPT ENABLE_GLSLANG_BINARIES ENABLE_SPVREMAPPER ENABLE_CTEST GLSLANG_TESTS
		GLSLANG_ENABLE_INSTALL ENABLE_HLSL BUILD_EXTERNAL ENABLE_PCH)
	set(${off} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(${GLSLANG_DIR} ${CMAKE_BINARY_DIR}/glslang EXCLUDE_FROM_ALL)
# find_package(glslang CONFIG) in the toolkit: these targets (ps5/cmake/glslang/glslangConfig.cmake)
set(glslang_DIR ${ps5}/cmake/glslang CACHE PATH "" FORCE)

# Takedawn's options, as the console has them: the enhancements layer and the
# settings overlay on, the button prompts rendered on the Mac (tools/build.sh)
set(BO3_ENHANCE ON CACHE BOOL "" FORCE)
set(BO3_OVERLAY ON CACHE BOOL "" FORCE)
set(XBOXRECOMP_VULKAN ON CACHE BOOL "" FORCE)

# The link: every object and archive CMake would hand the linker goes to
# PS5_Vulkan's recipe instead (RADV, the platform layer, the CRT, the fake SELF)
foreach(language C CXX)
	set(CMAKE_${language}_LINK_EXECUTABLE
		"bash ${ps5}/tools/link-title.sh ${CMAKE_BINARY_DIR} ${PS5_VULKAN_DIR} ${PS5_PAYLOAD_SDK} --whole-archive ${ps5_platform_archive} --no-whole-archive <OBJECTS> <LINK_LIBRARIES>")
endforeach()
# (the start-up constructor in the platform archive has no caller: hence whole)

# The platform archive is named by file in the link, so the game does not
# depend on it by itself (the game's target exists once its CMakeLists.txt has run)
function(ps5_late_fixes)
	add_dependencies(burnout3 ps5_platform)
	set_property(TARGET burnout3 APPEND PROPERTY LINK_DEPENDS ${ps5_platform_archive})
endfunction()
cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL ps5_late_fixes)
