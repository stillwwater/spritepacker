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

#ifndef SPACK_PROJECT_H
#define SPACK_PROJECT_H

#include <memory>
#include <cassert>

#include "SDL.h"
#include "atlas.h"

namespace spack {

class Project {
public:
    std::string filename;
    std::vector<std::pair<std::string, AtlasExporter>> exporters;
    std::vector<std::unique_ptr<Atlas>> atlases;
    size_t current_atlas = 0;

    const char *error_id = nullptr;
    std::string error_msg;

    Project();

    void LoadEmptyProject(SDL_Renderer *device);
    bool Load(SDL_Renderer *device, const std::string &file);
    bool Save() const;

    void RegisterExportFunc(const std::string &name, AtlasExporter fn);
    bool ExportAllAtlases() const;

    void AddAtlas(std::unique_ptr<Atlas> atlas);
    std::unique_ptr<Atlas> MakeEmptyAtlas(SDL_Renderer *device) const;
    const std::unique_ptr<Atlas> &GetAtlas() const;

    void Error(const char *id, const std::string &msg);
};

inline const std::unique_ptr<Atlas> &Project::GetAtlas() const {
    assert(current_atlas >= 0 && current_atlas < atlases.size());
    return atlases[current_atlas];
}

} // namespace spack

#endif // SPACK_PROJECT_H
