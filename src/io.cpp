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

#include "io.h"

#include <memory>
#include <filesystem>
#include <cstdio>
#include <cassert>
#include <sstream>

#include "SDL.h"
#include "atlas.h"

namespace spack {

std::string RenameWithExt(const std::string &filename, const std::string &ext) {
    auto n = filename.find_first_of('.');
    if (n != 0 && n != std::string::npos) {
        return filename.substr(0, n + 1) + ext;
    }
    return filename;
}

bool HasExtension(const std::string &filename, const std::string &ext) {
    if (filename.size() < ext.size()) {
        return false;
    }
    return filename.compare(filename.size()-ext.size(), ext.size(), ext) == 0;
}

std::string BasePath(const std::string &filename) {
    auto sep = filename.find_last_of("/\\");
    if (sep != std::string::npos) {
        return filename.substr(0, sep + 1);
    }
    return "./";
}

static std::string RelativePathRelative(const std::string &base,
                                        const std::string &filename) {
    auto base_path = BasePath(base);
    auto rel =  std::filesystem::relative(filename, BasePath(base)).u8string();
    return rel;
}

template <typename T>
static void ParseInt(T *opt, const std::string &expect,
                     const std::string &key, const std::string &value) {
    if (key == expect) {
        *opt = static_cast<T>(std::stoi(value));
    }
}

bool LoadProject(SDL_Renderer *device,
                 const std::string &filename,
                 std::vector<std::unique_ptr<Atlas>> *project) {
    auto *file = fopen(filename.c_str(), "r");
    if (file == nullptr) return false;
    fseek(file, 0, SEEK_END);
    auto size = ftell(file);
    fseek(file, 0, SEEK_SET);

    auto *buffer = new char[size];
    fread((void *)buffer, sizeof(char), size, file);

    std::string data(buffer, size);
    std::stringstream lines(data);
    std::string line;
    auto base = std::filesystem::absolute(BasePath(filename)).u8string();
    assert(project != nullptr);
    project->clear();

    while (std::getline(lines, line)) {
        if (line == "" || line == "\n" || line == "\r\n") continue;
        if (line[0] == '#') continue;

        auto sep = line.find(' ');
        if (sep == std::string::npos) {
            fclose(file);
            delete[] buffer;
            return false;
        }
        auto key = line.substr(0, sep);
        auto value = line.substr(sep + 1);

        if (key == "atlas") {
            project->push_back(std::make_unique<Atlas>(device));
            project->back()->output_file = value;
            continue;
        }
        if (project->size() == 0) {
            fclose(file);
            delete[] buffer;
            return false;
        }
        if (key == "image") {
            project->back()->output_image = value;
            continue;
        }
        if (key == "sprite") {
            project->back()->AppendSprite(base + value);
            continue;
        }
        ParseInt(&project->back()->padding_mode, "padding_mode", key, value);
        ParseInt(&project->back()->padding, "padding", key, value);
        ParseInt(&project->back()->normalize, "normalize", key, value);
        ParseInt(&project->back()->y_up, "y_up", key, value);
        ParseInt(&project->back()->square_texture, "square", key, value);
        ParseInt(&project->back()->image_format, "image_format", key, value);
    }
    delete[] buffer;
    fclose(file);
    for (auto &atlas : *project) {
        // Render all atlases on load
        atlas->Render();
    }
    return true;
}

bool SaveProject(const std::string &filename,
                 const std::vector<std::unique_ptr<Atlas>> &atlases) {
    auto *file = fopen(filename.c_str(), "w+");
    if (file == nullptr) return false;

    for (const auto &atlas : atlases) {
        fprintf(file, "atlas %s\n", atlas->output_file.c_str());
        fprintf(file, "image %s\n", atlas->output_image.c_str());
        fprintf(file, "image_format %d\n", atlas->image_format);
        fprintf(file, "square %d\n",atlas->square_texture);
        fprintf(file, "padding %d\n", atlas->padding);
        fprintf(file, "padding_mode %d\n", atlas->padding_mode);
        fprintf(file, "normalize %d\n", atlas->normalize);
        fprintf(file, "y_up %d\n", atlas->y_up);
        for (const auto &sprite : atlas->sprites) {
            fprintf(file, "sprite %s\n",
                    RelativePathRelative(filename, sprite.filename).c_str());
        }
    }
    fclose(file);
    return false;
}

bool ExportAtlasFile(const Atlas &atlas, const std::vector<SDL_FRect> &quads) {
    auto *file = fopen(atlas.output_file.c_str(), "w+");
    if (file == nullptr) return false;

    fprintf(file, "i %s\n", atlas.output_image.c_str());

    for (size_t i = 0; i < quads.size(); ++i) {
        auto &sprite = atlas.sprites[i];
        auto &quad = quads[i];

        fprintf(file, "s %s", sprite.short_name.c_str());
        if (atlas.normalize) {
           fprintf(file, " %f %f %f %f\n", quad.x, quad.y, quad.w, quad.h);
           continue;
        }
        fprintf(file, " %d %d %d %d\n",
                int(quad.x), int(quad.y), int(quad.w), int(quad.h));
    }
    fclose(file);
    return true;
}

bool ExportJson(const Atlas &atlas, const std::vector<SDL_FRect> &quads) {
    auto *file = fopen(atlas.output_file.c_str(), "w+");
    if (file == nullptr) return false;

    fprintf(file, "{\"texture\":\"%s\",", atlas.output_image.c_str());
    fprintf(file, "\"sprites\":[");

    for (size_t i = 0; i < quads.size(); ++i) {
        auto &sprite = atlas.sprites[i];
        auto &quad = quads[i];

        if (i > 0) fprintf(file, ",");
        fprintf(file, "{\"name\":\"%s\",", sprite.short_name.c_str());
        fprintf(file, "\"x\":%f,\"y\":%f,\"w\":%f,\"h\":%f}",
                quad.x, quad.y, quad.w, quad.h);
    }
    fprintf(file, "]}\n");
    fclose(file);
    return true;
}

} // namespace spack
