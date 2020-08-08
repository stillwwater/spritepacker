# Copyright (c) 2020 stillwwater
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
	message(FATAL_ERROR "Prevented in-tree build.")
endif()

cmake_minimum_required(VERSION 3.10)

project(spritepacker)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    set(CMAKE_CXX_WARNING_LEVEL 4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /GR-")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fno-exceptions -fno-rtti")
endif()

#
# Modules
#

set(TWO_SRC_MODULES
    src/atlas.cpp
    src/atlas.h
    src/image.h
    src/image.cpp
    src/io.cpp
    src/io.h
    src/project.cpp
    src/project.h
    src/spritepacker.cpp
    src/ui.cpp
    src/ui.h
)

include_directories(
    src
    external
)
add_executable(spritepacker ${TWO_SRC_MODULES})

#
# Third party libraries
#

#
# SDL2
#

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
add_subdirectory(external/SDL2)
include_directories(external/SDL2/include)

#
# stb
#

set(SP_SRC_3P_STB
    external/stb/stb_image.h
    external/stb/stb_image.c
    external/stb/stb_image_write.h
    external/stb/stb_image_write.c
)
# Adding stb as a separate library so the compiler doesn't treat stb as
# C++ code and give us a bunch of warnings.
add_library(sp_3p_stb ${SP_SRC_3P_STB})

#
# lodepng
#

set(SP_SRC_3P_LODEPNG
    external/lodepng/lodepng.h
    external/lodepng/lodepng.cpp
)
add_library(sp_3p_lodepng ${SP_SRC_3P_LODEPNG})

#
# ImGui
#

set(SP_SRC_3P_IMGUI
    external/imgui/imconfig.h
    external/imgui/imgui.cpp
    external/imgui/imgui.h
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_impl_sdl.cpp
    external/imgui/imgui_impl_sdl.h
    external/imgui/imgui_internal.h
    external/imgui/imgui_widgets.cpp
    external/imgui/imstb_rectpack.h
    external/imgui/imstb_textedit.h
    external/imgui/imstb_truetype.h
    external/imgui/imgui_stdlib.cpp
    external/imgui/imgui_stdlib.h
    external/imgui/imgui_sdl.h
    external/imgui/imgui_sdl.cpp
)
add_library(sp_3p_imgui ${SP_SRC_3P_IMGUI})

set(SP_3P_SDL
    SDL2main
    SDL2-static
)
set(SP_3P
    ${SP_3P_SDL}
    sp_3p_imgui
    sp_3p_stb
    sp_3p_lodepng
)

target_link_libraries(sp_3p_imgui ${SP_SP_SDL})
target_link_libraries(spritepacker ${SP_3P})
