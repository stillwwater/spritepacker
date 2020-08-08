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

#include "image.h"

#include <cassert>
#include <vector>
#include <string>

#include "SDL.h"
#include "lodepng/lodepng.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

namespace spack {

const char *ImageExt[] {"png", "tga", "bmp"};

static std::string BaseSpriteName(const std::string &filename) {
    std::string result = filename;
    auto sep = result.find_last_of("/\\");
    if (sep != std::string::npos) {
        result = result.substr(sep + 1);
    }
    auto ext = result.find_first_of('.');
    if (ext != 0 && ext != std::string::npos) {
        result = result.substr(0, ext);
    }
    return result;
}

std::optional<Sprite> LoadSprite(SDL_Renderer *device,
                                 const std::string &filename) {
    int w, h, comp;
    auto *im = stbi_load(filename.c_str(), &w, &h, &comp, STBI_default);
    if (im == nullptr) return {};

    int fmt;
    switch (comp) {
    case 3:
        fmt = SDL_PIXELFORMAT_RGB24;
        break;
    case 4:
        fmt = SDL_PIXELFORMAT_RGBA32;
        break;
    default:
        return {};
    }

    Sprite sprite;
    sprite.filename = filename;
    sprite.short_name = BaseSpriteName(filename);
    sprite.rect = SDL_Rect{0, 0, w, h};
    sprite.texture = SDL_CreateTexture(device, fmt,
                                       SDL_TEXTUREACCESS_STATIC, w, h);

    int pitch = comp * w;
    SDL_UpdateTexture(sprite.texture, &sprite.rect, (const void *)im, pitch);
    SDL_SetTextureBlendMode(sprite.texture, SDL_BLENDMODE_BLEND);

    stbi_image_free(im);
    return sprite;
}

void WriteTexture(SDL_Renderer *device, const std::string &filename,
                  ImageFormat image_fmt, SDL_Texture *tex) {
    uint32_t fmt;
    int access;
    int w, h;

    SDL_QueryTexture(tex, &fmt, &access, &w, &h);
    assert(access == SDL_TEXTUREACCESS_TARGET);
    assert(fmt == SDL_PIXELFORMAT_RGBA32);

    int pitch = 4 * w;
    auto image = new unsigned char[pitch * h];

    SDL_SetRenderTarget(device, tex);
    SDL_RenderReadPixels(device, nullptr, fmt, (void *)image, pitch);
    SDL_SetRenderTarget(device, nullptr);

    switch (image_fmt) {
    case Image_PNG:
        lodepng_encode32_file(filename.c_str(), image, w, h);
        break;
    case Image_TGA:
        stbi_write_tga(filename.c_str(), w, h, 4, (const void *)image);
        break;
    case Image_BMP:
        stbi_write_bmp(filename.c_str(), w, h, 4, (const void *)image);
        break;
    default:
        assert(false && "Invalid image format");
    }
    delete[] image;
}

RenderSprite MakeRenderSprite(SDL_Renderer *device, const Sprite &sprite,
                              int padding, PaddingMode mode) {
    int p = padding;
    RenderSprite result;
    result.src = {0, 0, sprite.rect.w + 2*p, sprite.rect.h + 2*p};
    result.texture = SDL_CreateTexture(device, SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_TARGET,
                                       result.src.w, result.src.h);
    SDL_SetTextureBlendMode(result.texture, SDL_BLENDMODE_BLEND);

    SDL_Rect bleed_src[8] = {
        // Corners
        {0, 0, 1, 1},
        {sprite.rect.w - 1, 0, 1, 1},
        {sprite.rect.w - 1, sprite.rect.h - 1, 1, 1},
        {0, sprite.rect.h - 1, 1, 1},
        // Edges
        {0, 0, sprite.rect.w, 1},
        {sprite.rect.w - 1, 0, 1, sprite.rect.h},
        {0, sprite.rect.h - 1, sprite.rect.w, 1},
        {0, 0, 1, sprite.rect.h}
    };

    SDL_Rect bleed_dst[8] = {
        // Corners
        {0, 0, p, p},
        {sprite.rect.w + p, 0, p, p},
        {sprite.rect.w + p, sprite.rect.h + p, p, p},
        {0, sprite.rect.h + p, p, p},
        // Edges
        {p, 0, sprite.rect.w, p},
        {sprite.rect.w + p, p, p, sprite.rect.h},
        {p, sprite.rect.h + p, sprite.rect.w, p},
        {0, p, p, sprite.rect.h}
    };

    // Main dst
    SDL_Rect dst{p, p, sprite.rect.w, sprite.rect.h};
    SDL_SetRenderTarget(device, result.texture);
    SDL_SetRenderDrawColor(device, 0, 0, 0, 0);
    SDL_RenderClear(device);

    for (int i = 0; i < 8; ++i) {
        switch (mode) {
        case Padding_Bleed:
            SDL_RenderCopy(device, sprite.texture,
                           &bleed_src[i], &bleed_dst[i]);
            break;
        case Padding_Alpha:
            SDL_RenderFillRect(device, &bleed_dst[i]);
            break;
        case Padding_Debug:
            SDL_SetRenderDrawColor(device, 255, 255, 0, 255);
            SDL_RenderFillRect(device, &bleed_dst[i]);
            break;
        default:
            assert(false && "Invalid padding mode");
        }
    }
    SDL_RenderCopy(device, sprite.texture, &sprite.rect, &dst);
    SDL_SetRenderTarget(device, nullptr);
    return result;
}

} // namespace spack
