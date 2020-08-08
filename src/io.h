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

#ifndef SPACK_IO_H
#define SPACK_IO_H

#include <memory>

#include "SDL.h"
#include "atlas.h"

namespace spack {

std::string RenameWithExt(const std::string &filename, const std::string &ext);
bool HasExtension(const std::string &filename, const std::string &ext);
std::string BasePath(const std::string &filename);

bool LoadProject(SDL_Renderer *device,
                 const std::string &filename,
                 std::vector<std::unique_ptr<Atlas>> *project);

bool SaveProject(const std::string &filename,
                 const std::vector<std::unique_ptr<Atlas>> &atlases);

bool ExportAtlasFile(const Atlas &atlas, const std::vector<SDL_FRect> &quads);
bool ExportJson(const Atlas &atlas, const std::vector<SDL_FRect> &quads);

} // namespace spack

#endif // SPACK_IO_H
