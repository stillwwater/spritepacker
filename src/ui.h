// Copyright (c) 2020 stillwwater
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#ifndef SPACK_UI_H
#define SPACK_UI_H

#include "SDL.h"
#include "project.h"
#include "imgui/imgui.h"

namespace spack {

constexpr int DefaultWindowW = 1280;
constexpr int DefaultWindowH = 768;

SDL_Renderer *MakeDefaultRenderer(SDL_Window *window);
SDL_Window *MakeDefaultWindow(bool headless = false);
void InitInput(ImGuiIO *io);

void RenderUi(SDL_Renderer *deice, Project *project);
void ProcessEvent(SDL_Renderer *device, Project *project, const SDL_Event &e);
void MainLoop(SDL_Renderer *device, Project *project);

} // namespace spack

#endif // SPACK_UI_H
