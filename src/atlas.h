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

#ifndef SPACK_ATLAS_H
#define SPACK_ATLAS_H

#include <vector>
#include <string>

#include "SDL.h"
#include "image.h"

namespace spack {

using AtlasExporter = bool (*)(const class Atlas &atlas,
                               const std::vector<SDL_FRect> &quads);

class Atlas {
public:
    int width, height;
    std::vector<Sprite> sprites;
    SDL_Texture *texture = nullptr;

    std::string output_file = "untitled.atlas";
    std::string output_image = "untitled.png";

    ImageFormat image_format = Image_PNG;
    size_t exporter = 0;

    // Render state used for rendering UI
    SDL_Point position{0, 0};
    SDL_Point origin{0, 0};
    float scale = 1.0f;

    // Padding in pixels in between each sprite
    int padding = 0;
    PaddingMode padding_mode = Padding_Bleed;

    // Pad the atlas to have atlas width equal to the atlas height.
    bool square_texture = false;

    // Use normalized coordinates for the sprite rects. If this is true
    // the sprite rects will have their position between (0, 0) and (1, 1)
    // as opposed to (0, 0) to (atlas_width, atlas_height). Most 3D renderers
    // require this for texture UVs.
    bool normalize = false;

    // Use "OpenGL style" coordinates with (0, 0) at the bottom left corner,
    // default is (0, 0) at the top left.
    bool y_up = false;

    Atlas(SDL_Renderer *device) : device(device) {}
    ~Atlas();

    Atlas(const Atlas &) = delete;
    Atlas &operator=(const Atlas &) = delete;

    bool AppendSprite(const std::string &filename);
    void AppendSprite(const Sprite &sprite);

    void RenderSprites();

    SDL_Point PackedSize(int heur = 0, int n = 0) const;
    SDL_Point Pack(int heur = 0, int n = 0);

    void CreateTexture(int w, int h);
    void Render();

    bool Export(AtlasExporter fn);
    void SetZoom(float value);

private:
    SDL_Renderer *device;
    std::vector<RenderSprite> render_sprites;
};

} // namespace spack

#endif // SPACK_ATLAS_H
