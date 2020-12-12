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

#include <cstdio>
#include <cstring>

#include "SDL.h"
#include "imgui/imgui_sdl.h"
#include "imgui/imgui.h"

#include "ui.h"
#include "project.h"

int UiMain(SDL_Renderer *device, const char *filename = nullptr) {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiSDL::Initialize(device, spack::DefaultWindowW, spack::DefaultWindowH);

    auto &io = ImGui::GetIO();
    spack::InitInput(&io);
    io.IniFilename = nullptr;

    spack::Project project;
    if (filename == nullptr || !project.Load(device, filename)) {
        project.LoadEmptyProject(device);
    }
    spack::MainLoop(device, &project);

    ImGuiSDL::Deinitialize();
    ImGui::DestroyContext();
    return 0;
}

int CliMain(SDL_Renderer *device, const char *filename) {
    spack::Project project;
    if (!project.Load(device, filename)) {
        fprintf(stderr, "error: Failed to load project %s\n", filename);
        return 1;
    }
    if (!project.ExportAllAtlases()) {
        fprintf(stderr, "error: Failed to export atlases\n");
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "error: %s\n", SDL_GetError());
        return 1;
    }
    bool headless = argc > 2 && strcmp(argv[1], "-export") == 0;
    auto *window = spack::MakeDefaultWindow(headless);
    auto *device = spack::MakeDefaultRenderer(window);
    int error;

    if (headless)
        error = CliMain(device, argv[2]);
    else if (argc > 1)
        error = UiMain(device, argv[1]);
    else
        error = UiMain(device);

    SDL_DestroyRenderer(device);
    SDL_DestroyWindow(window);
    return error;
}
