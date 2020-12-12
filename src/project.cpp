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

#include "project.h"

#include <memory>
#include <cassert>

#include "SDL.h"
#include "atlas.h"
#include "io.h"

namespace spack {

Project::Project() {
    RegisterExportFunc("atlas", &ExportAtlasFile);
    RegisterExportFunc("txt", &ExportAtlasFile);
    RegisterExportFunc("json", &ExportJson);
}

void Project::LoadEmptyProject(SDL_Renderer *device) {
    filename = "untitled.spritepack";
    atlases.clear();
    AddAtlas(MakeEmptyAtlas(device));
    current_atlas = 0;
}

bool Project::Load(SDL_Renderer *device, const std::string &file) {
    bool ok = LoadProject(device, file, &atlases);
    if (!ok || atlases.size() == 0) {
        LoadEmptyProject(device);
        return false;
    }
    filename = file;
    current_atlas = 0;
    atlases[current_atlas]->Render();
    return true;
}

bool Project::Save() const {
    return SaveProject(filename, atlases);
}

void Project::RegisterExportFunc(const std::string &name, AtlasExporter fn) {
    exporters.push_back(std::make_pair(name, fn));
}

bool Project::ExportAllAtlases() const {
    bool error = false;
    for (const auto &atlas : atlases) {
        assert(atlas->exporter >= 0 && atlas->exporter < exporters.size());
        error = !atlas->Export(exporters[atlas->exporter].second) || error;
    }
    return !error;
}

std::unique_ptr<Atlas> Project::MakeEmptyAtlas(SDL_Renderer *device) const {
    auto atlas = std::make_unique<Atlas>(device);
    atlas->CreateTexture(128, 128);

    Animation none{};
    none.name = "<none>";
    atlas->animations.push_back(std::move(none));
    return atlas;
}

void Project::AddAtlas(std::unique_ptr<Atlas> atlas) {
    atlases.push_back(std::move(atlas));
}

void Project::Error(const char *id, const std::string &msg) {
    error_id = id;
    error_msg = msg;
}

} // namespace spack
