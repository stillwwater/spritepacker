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

#include "atlas.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cmath>

#include "SDL.h"
#include "image.h"

namespace spack {

static uint32_t NextPow2(uint32_t value) {
    --value;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return value + 1;
}

Atlas::~Atlas() {
    // Make sure atlas destructor is not called after SDL_DestroyRenderer!
    if (device != nullptr && texture != nullptr) {
        SDL_DestroyTexture(texture);
        FreeSpriteTextures(sprites);
        FreeSpriteTextures(render_sprites);
    }
}

void Atlas::CreateTexture(int w, int h) {
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }
    texture = SDL_CreateTexture(device,
                                SDL_PIXELFORMAT_RGBA32,
                                SDL_TEXTUREACCESS_TARGET,
                                w, h);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(device, texture);
    SDL_SetRenderDrawColor(device, 0, 0, 0, 0);
    SDL_RenderClear(device);
    SDL_SetRenderTarget(device, nullptr);
    width = w;
    height = h;
}

// Returns approximate power of 2 packing size
// This size assumes sprites can be packed without any wasted space,
// which may not be the case so min_h is a heuristic value used to
// adjust the height until all sprites fit.
SDL_Point Atlas::PackedSize(int heur, int n) const {
    int area = 0;
    int max_w = 0;
    int max_h = 0;

    for (const auto &sprite : render_sprites) {
        area += sprite.src.w * sprite.src.h;
        max_w = std::max(max_w, sprite.src.w);
        max_h = std::max(max_h, sprite.src.h);
    }

    int a = ceilf(sqrtf(area));
    int w = NextPow2(std::max(a, max_w));
    int h = NextPow2(std::max(std::max(a, heur), max_h));

    if (square_texture) {
        return SDL_Point{h, h};
    }

    // Try to use half the width
    if (n <= 1 && w == h && (w / 2) * h > area) {
        w /= 2;
    }

    // Try to use half the height. This is used to correct a
    // mistake that can be made by halving the width, where half the
    // width causes more wasted space. We stop if n > 1 and assume
    // this is not possible.
    if (n <= 1 && w != h && (h / 2) * w > area) {
        h /= 2;
    }
    return SDL_Point{w, h};
}

SDL_Point Atlas::Pack(int heur, int n) {
    assert(n < 512);
    std::sort(render_sprites.begin(), render_sprites.end(),
        [](const RenderSprite &a, const RenderSprite &b) {
            return a.src.w * a.src.h > b.src.w * b.src.h;
        });

    auto size = PackedSize(heur, n);
    auto *mask = new unsigned char[size.x * size.y];
    memset(mask, 0, size.x * size.y * sizeof(bool));

    for (auto &sprite : render_sprites) {
        int ox = 0;
        int oy = 0;
        bool packed = false;
        for (oy = 0; ; ++oy) {
            if (oy + sprite.src.h > size.y) {
                // Need more space
                delete[] mask;
                return Pack(size.y + 1, ++n);
            }
            for (ox = 0; ox <= size.x - sprite.src.w; ++ox) {
                unsigned char c = 0;
                // Rect collisions
                c |= mask[ox + oy * size.x];
                c |= mask[ox + (oy + sprite.src.h - 1) * size.x];
                c |= mask[(ox + sprite.src.w - 1) + oy * size.x];
                c |= mask[(ox+sprite.src.w-1) + (oy+sprite.src.h-1)*size.x];
                if (c == 0) {
                    packed = true;
                    break;
                }
            }
            if (packed) break;
        }
        for (int y = oy; y < oy + sprite.src.h; ++y) {
            for (int x = ox; x < ox + sprite.src.w; ++x) {
                mask[x + y * size.x] = 255;
            }
        }
        sprite.dst = SDL_Rect{ox, oy, sprite.src.w, sprite.src.h};
    }
    delete[] mask;
    return size;
}

void Atlas::Render() {
    if (render_sprites.size() == 0) {
        return;
    }
    auto size = Pack();
    if (size.x != width || size.y != height) {
        CreateTexture(size.x, size.y);
    }
    SDL_SetRenderTarget(device, texture);
    SDL_SetRenderDrawColor(device, 0, 0, 0, 0);
    SDL_RenderClear(device);

    for (const auto &sprite : render_sprites) {
        SDL_RenderCopy(device, sprite.texture,
                       &sprite.src, &sprite.dst);
    }
    SDL_SetRenderTarget(device, nullptr);
}

void Atlas::RenderSprites() {
    FreeSpriteTextures(render_sprites);
    render_sprites.clear();
    render_sprites.reserve(sprites.size());
    for (size_t i = 0; i < sprites.size(); ++i) {
        auto rs = MakeRenderSprite(device, sprites[i], padding, padding_mode);
        rs.sorting_order = i;
        render_sprites.push_back(std::move(rs));
    }
}

void Atlas::AppendSprite(const Sprite &sprite, int anim) {
    if (animations.size() == 0) {
        Animation none{};
        none.name = "<none>";
        animations.push_back(std::move(none));
    }
    assert(anim >= 0 && anim < animations.size());

    auto rs = MakeRenderSprite(device, sprite, padding, padding_mode);
    rs.sorting_order = sprites.size();
    animations[anim].frames.push_back(sprites.size());
    sprites.push_back(sprite);
    render_sprites.push_back(std::move(rs));
}

bool Atlas::AppendSprite(const std::string &filename, int anim) {
    auto sprite = LoadSprite(device, filename);
    if (!sprite.has_value())
        return false;
    AppendSprite(sprite.value(), anim);
    return true;
}

bool Atlas::Export(AtlasExporter fn) {
    RenderSprites();
    Render();
    if (render_sprites.size() == 0 || texture == nullptr) {
        return false;
    }

    std::sort(render_sprites.begin(), render_sprites.end(),
        [](const RenderSprite &a, const RenderSprite &b) {
            return a.sorting_order < b.sorting_order;
        });
    std::vector<SDL_FRect> quads;
    quads.reserve(render_sprites.size());

    for (const auto &sprite : render_sprites) {
        SDL_FRect quad{float(sprite.dst.x + padding),
                       float(sprite.dst.y + padding),
                       float(sprite.dst.w - padding * 2.0f),
                       float(sprite.dst.h - padding * 2.0f)};
        if (y_up) {
            quad.y = height - quad.y - quad.h;
        }
        if (normalize) {
            quad.x /= width;
            quad.y /= height;
        }
        quads.push_back(quad);
    }
    bool ok = fn(*this, quads);
    WriteTexture(device, output_image, image_format, texture);
    return ok;
}

void Atlas::SetZoom(float value) {
    scale += value * 0.25f;
    scale = fminf(scale, 4.0f);
}

} // namespace spack
