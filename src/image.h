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

#ifndef SPACK_IMAGE_H
#define SPACK_IMAGE_H

#include <vector>
#include <string>
#include <optional>

#include "SDL.h"

namespace spack {

struct Sprite {
    std::string filename;
    std::string short_name;
    SDL_Rect rect;
    SDL_Texture *texture;
};

struct RenderSprite {
    SDL_Rect src;
    SDL_Rect dst;
    SDL_Texture *texture;
    int sorting_order;
};

enum PaddingMode {
    Padding_Bleed,
    Padding_Alpha,
    Padding_Debug,
};

enum ImageFormat {
    Image_PNG,
    Image_TGA,
    Image_BMP,
};

extern const char *ImageExt[];

std::optional<Sprite> LoadSprite(SDL_Renderer *device,
                                 const std::string &filename);

void WriteTexture(SDL_Renderer *device, const std::string &filename,
                  ImageFormat image_fmt, SDL_Texture *tex);

RenderSprite MakeRenderSprite(SDL_Renderer *device, const Sprite &sprite,
                              int padding, PaddingMode mode);

template <typename T>
void FreeSpriteTextures(const std::vector<T> &sprites) {
    if (sprites.size() == 0) return;
    for (const auto &sprite : sprites) {
        SDL_DestroyTexture(sprite.texture);
    }
}

} // namespace spack

#endif // SPACK_IMAGE_H
