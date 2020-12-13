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

#include "ui.h"

#include <functional>
#include <memory>
#include <chrono>
#include <cmath>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/imgui_sdl.h"
#include "SDL.h"
#include "atlas.h"
#include "io.h"
#include "project.h"

namespace spack {

constexpr char Help_Normalize[] =
    "Normalize sprite coordinates in the atlas texture to be between [0, 1)"
    " instead of using pixel coordinates.";

constexpr char Help_YUp[] =
    "Use 'OpenGL style' coordinates with (0, 0) at the bottom left corner,"
    " default is (0, 0) at the top left.";

constexpr char Help_FrameTime[] = "Duration of each frame in seconds.";

#ifdef __APPLE__
constexpr char Help_Save[] = "Save (Command + S)";
constexpr char Help_Export[] = "Export (Command + E)";
#else
constexpr char Help_Save[] = "Save (Ctrl + S)";
constexpr char Help_Export[] = "Export (Ctrl + E)";
#endif

constexpr char Error_InvalidImage[] = "Invalid file format";
constexpr char ErrorM_InvalidImage[] = "Could not open %s";

SDL_Renderer *MakeDefaultRenderer(SDL_Window *window) {
    uint32_t flags = SDL_RENDERER_ACCELERATED
                   | SDL_RENDERER_PRESENTVSYNC
                   | SDL_RENDERER_TARGETTEXTURE;

	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    return SDL_CreateRenderer(window, 0, flags);
}

SDL_Window *MakeDefaultWindow(bool headless) {
    auto flags = SDL_WINDOW_ALLOW_HIGHDPI
               | SDL_WINDOW_RESIZABLE;

    // SDL_WINDOW_HIDDEN allows for true headless mode but some graphics
    // operations seem to do nothing using SDL_Renderer, so we just run the
    // window minimized for now but this won't work without a GPU.
    if (headless)
        flags |= SDL_WINDOW_MINIMIZED;

    return SDL_CreateWindow("Sprite Packer",
                            SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED,
                            DefaultWindowW, DefaultWindowH, flags);
}

void InitInput(ImGuiIO *io) {
    io->KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    io->KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io->KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io->KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io->KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io->KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io->KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io->KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io->KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io->KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
    io->KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    io->KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    io->KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
    io->KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    io->KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    io->KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
    io->KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    io->KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    io->KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    io->KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    io->KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    io->KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
}

static void Section(const char *text) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text("%s", text);
    ImGui::Spacing();
}

static void DrawTooltip(const char *text) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void DrawNoSpritesInfo() {
    auto &io = ImGui::GetIO();
    ImVec2 size{200.0f, 60.0f};
    ImVec2 pos{io.DisplaySize.x * 0.5f - size.x * 0.5f,
               io.DisplaySize.y * 0.5f - size.y * 0.5f};

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);

    auto flags = ImGuiWindowFlags_NoDecoration
               | ImGuiWindowFlags_NoFocusOnAppearing
               | ImGuiWindowFlags_NoNav;
    ImGui::Begin("Overlay", nullptr, flags);
    ImGui::Text("Drop PNG/TGA/BMP images or\n"
                "project files (.spritepack)\nhere.");
    ImGui::End();
}

static std::string UniqueLabel(const std::string &label, size_t key) {
    // Not great having to do this but ImGui depends on labels
    // to identify selectables in lists.
    return label + "##" + std::to_string(key);
}

template <typename... Args>
static void DrawMessageDialog(const char *name,
                              const char *fmt, Args &&...args) {
    ImGui::SetNextWindowSize(ImVec2(300, 128));
    if (ImGui::BeginPopupModal(name, nullptr, ImGuiWindowFlags_NoResize)) {
        ImGui::TextWrapped(fmt, args...);
        ImGui::Dummy(ImVec2(0, 20));
        if (ImGui::Button("Close", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

static void DrawAnimationsWindow(const Project &project) {
    auto &atlas = project.GetAtlas();

    ImGui::SetNextWindowBgAlpha(0.9f);
    ImGui::Begin("Animation Groups", nullptr, ImGuiWindowFlags_NoResize);
    size_t new_selected = atlas->selected_anim;

    if (ImGui::Button("New Animation")) {
        Animation anim{};
        anim.name = "untitled_anim";

        if (atlas->animations.size() > 0) {
            anim.frame_time = atlas->animations.back().frame_time;
        }
        atlas->animations.push_back(anim);
        ++new_selected;
    }
    ImGui::SameLine();

    // Cannot remove the default animation group
    if (ImGui::Button("Remove") && new_selected < atlas->animations.size()
            && new_selected > 0) {
        for (int removed_frame : atlas->animations[new_selected].frames) {
            auto &sprite = atlas->sprites[removed_frame];
            SDL_DestroyTexture(sprite.texture);
            atlas->sprites.erase(atlas->sprites.begin() + removed_frame);

            for (auto &anim : atlas->animations) {
                for (int &frame : anim.frames) {
                    if (frame >= removed_frame)
                        --frame;
                }
            }
        }
        atlas->animations.erase(atlas->animations.begin() + new_selected);
        atlas->RenderSprites();
        atlas->Render();
        --new_selected;
    }

    for (size_t i = 0; i < atlas->animations.size(); ++i) {
        auto &anim = atlas->animations[i];
        auto label = UniqueLabel(anim.name, i);

        if (ImGui::Selectable(label.c_str(), i == new_selected))
            new_selected = i;
    }

    if (new_selected != atlas->selected_anim) {
        atlas->selected_anim = new_selected;
        atlas->selected_sprite = 0;
    }

    ImGui::End();
}

static void DrawSpritesWindow(const Project &project) {
    auto &atlas = project.GetAtlas();
    auto &sprites = atlas->animations[atlas->selected_anim].frames;

    ImGui::SetNextWindowBgAlpha(0.9f);
    ImGui::Begin("Sprites", nullptr, ImGuiWindowFlags_NoResize);

    if (ImGui::Button("Up") && atlas->selected_sprite > 0 && sprites.size() > 0) {
        std::swap(sprites[atlas->selected_sprite],
                  sprites[atlas->selected_sprite - 1]);
        --atlas->selected_sprite;
    }
    ImGui::SameLine();

    if (ImGui::Button("Down") && atlas->selected_sprite < sprites.size() - 1
            && sprites.size() > 0) {
        std::swap(sprites[atlas->selected_sprite],
                  sprites[atlas->selected_sprite + 1]);
        ++atlas->selected_sprite;
    }
    ImGui::SameLine();

    if (ImGui::Button("Remove") && atlas->selected_sprite < sprites.size()
            && sprites.size() > 0) {
        int removed_frame = sprites[atlas->selected_sprite];
        auto &sprite = atlas->sprites[removed_frame];

        SDL_DestroyTexture(sprite.texture);

        sprites.erase(sprites.begin() + atlas->selected_sprite);
        atlas->sprites.erase(atlas->sprites.begin() + removed_frame);

        for (auto &anim : atlas->animations) {
            for (int &frame : anim.frames) {
                if (frame >= removed_frame)
                    --frame;
            }
        }
        atlas->RenderSprites();
        atlas->Render();

        if (atlas->selected_sprite > 0) --atlas->selected_sprite;
    }

    if (atlas->selected_anim > 0) {
        auto &anim = atlas->animations[atlas->selected_anim];
        ImGui::InputText("Name", &anim.name);
        ImGui::SetNextItemWidth(100);
        ImGui::InputFloat("Frame Time", &anim.frame_time, 0.0f, 0.0f, "%.3f");

        if (anim.frame_time > 0.0f && anim.frame_time < 1.0f) {
            ImGui::SameLine();
            ImGui::TextDisabled("(%dfps)", int(1.0f / anim.frame_time));
        }
        DrawTooltip(Help_FrameTime);
    }

    for (size_t i = 0; i < sprites.size(); ++i) {
        int frame = sprites[i];
        const auto &sprite = atlas->sprites[frame];
        auto label = UniqueLabel(sprite.short_name, i);

        ImGui::Image(sprite.texture, ImVec2(20, 20));
        ImGui::SameLine();

        ImGui::Text("%03d ", int(i));
        ImGui::SameLine();

        if (ImGui::Selectable(label.c_str(), i == atlas->selected_sprite))
            atlas->selected_sprite = i;
    }
    ImGui::End();
}

template <typename T>
static void DrawOption(const std::unique_ptr<Atlas> &atlas, T *option,
                       const std::function<void(T *opt)> &fn) {
    T opt = *option;
    fn(&opt);
    if (opt != *option) {
        *option = opt;
        atlas->RenderSprites();
        atlas->Render();
    }
}

static void DrawAtlasWindow(const Project &project) {
    auto &atlas = project.GetAtlas();
    static std::string tmp_str = atlas->output_file;

    ImGui::SetNextWindowBgAlpha(0.9f);
    ImGui::Begin("Atlas", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::Text("%dx%d", atlas->width, atlas->height);

    DrawOption<bool>(atlas, &atlas->square_texture, [](bool *opt) {
        ImGui::Checkbox("Square Texture", opt);
    });

    Section("Padding");
    DrawOption<int>(atlas, &atlas->padding, [](int *opt) {
        ImGui::SliderInt("Size", opt, 0, 8, "%dpx");
    });

    DrawOption<PaddingMode>(atlas, &atlas->padding_mode, [](PaddingMode *m) {
        const char *labels[3] = {"Bleed", "Alpha", "Debug"};
        int selected = static_cast<int>(*m);

        if (!ImGui::BeginCombo("Mode", labels[selected])) return;
        for (int i = 0; i < 3; ++i) {
            if (ImGui::Selectable(labels[i], i == selected))
                *m = static_cast<PaddingMode>(i);
        }
        ImGui::EndCombo();
    });

    Section("Export");
    DrawOption<bool>(atlas, &atlas->normalize, [](bool *opt) {
        ImGui::Checkbox("Normalize Coordinates", opt);
        DrawTooltip(Help_Normalize);
    });
    DrawOption<bool>(atlas, &atlas->y_up, [](bool *opt) {
        ImGui::Checkbox("Y Up", opt);
        DrawTooltip(Help_YUp);
    });

    ImGui::Spacing();
    ImGui::Text("Atlas");
    DrawOption<size_t>(atlas, &atlas->exporter, [&project](size_t *n) {
        assert(*n < project.exporters.size());
        auto &selected = project.exporters[*n].first;
        if (!ImGui::BeginCombo("Format##Atlas", selected.c_str())) return;
        for (size_t i = 0; i < project.exporters.size(); ++i) {
            auto &name = project.exporters[i].first;
            if (ImGui::Selectable(name.c_str(), i == *n)) {
                auto &a = project.GetAtlas();
                a->output_file = RenameWithExt(a->output_file, name);
                *n = i;
            }
        }
        ImGui::EndCombo();
    });
    ImGui::InputText("Path##Atlas", &atlas->output_file);

    ImGui::Spacing();
    ImGui::Text("Texture");
    DrawOption<ImageFormat>(atlas, &atlas->image_format,
        [&atlas](ImageFormat *f) {
            int selected = static_cast<int>(*f);
            if (!ImGui::BeginCombo("Format##Texture", ImageExt[selected]))
                return;
            for (int i = 0; i < 3; ++i) {
                if (ImGui::Selectable(ImageExt[i], i == selected)) {
                    *f = static_cast<ImageFormat>(i);
                    atlas->output_image = RenameWithExt(
                            atlas->output_image, ImageExt[i]);
                }
            }
            ImGui::EndCombo();
        });
     ImGui::InputText("Path##Texture", &atlas->output_image);

    if (ImGui::Button("Export")) {
        atlas->Export(project.exporters[atlas->exporter].second);
    }
    DrawTooltip(Help_Export);
    ImGui::End();
}

static void DrawProjectWindow(SDL_Renderer *device, Project *project) {
    static size_t selected = 0;
    ImGui::SetNextWindowBgAlpha(0.9f);
    ImGui::Begin("Project", nullptr, ImGuiWindowFlags_NoResize);

    ImGui::InputText("Path", &project->filename);
    if (ImGui::Button("New Project")) {
        project->LoadEmptyProject(device);
        selected = 0;
    }
    ImGui::SameLine();

    if (ImGui::Button("Load")) {
        project->Load(device, project->filename);
    }
    ImGui::SameLine();

    if (ImGui::Button("Save")) {
        project->Save();
    }
    DrawTooltip(Help_Save);

    Section("Atlases");
    if (ImGui::Button("New Atlas")) {
        selected = project->atlases.size();
        project->AddAtlas(project->MakeEmptyAtlas(device));
        project->current_atlas = selected;
    }
    ImGui::SameLine();

    if (ImGui::Button("Remove")) {
        project->atlases.erase(project->atlases.begin() + selected);
        if (selected > 0) --selected;
        if (project->atlases.size() == 0) {
            project->AddAtlas(project->MakeEmptyAtlas(device));
        }
        project->current_atlas = selected;
    }
    ImGui::SameLine();

    if (ImGui::Button("Export All")) {
        project->ExportAllAtlases();
    }

    for (size_t i = 0; i < project->atlases.size(); ++i) {
        const auto &a = project->atlases[i];
        auto label = UniqueLabel(a->output_file, i);

        if (ImGui::Selectable(label.c_str(), i == selected)) {
            selected = i;
            project->current_atlas = selected;
        }
    }
    ImGui::End();
}

static void DrawBackground(SDL_Renderer *device, const SDL_Rect &rect) {
    constexpr int CellSize = 16;
    const uint8_t Colors[] = {0x80, 0xc0};

    int dw, dh;
    SDL_GetRendererOutputSize(device, &dw, &dh);
    int width = rect.w / CellSize;
    int height = rect.h / CellSize;

    for (int y = 0; y <= height; ++y) {
        for (int x = 0; x <= width; ++x) {
            SDL_Rect dst{x * CellSize, y * CellSize,
                         CellSize, CellSize};
            if (x == width) {
                dst.w = rect.w - dst.x;
            }
            if (y == height) {
                dst.h = rect.h - dst.y;
            }
            dst.x += rect.x;
            dst.y += rect.y;

            if (dst.x + dst.w < 0 || dst.y + dst.h < 0
                    || dst.x > dw || dst.h > dh) {
                // Culling
                continue;
            }
            auto color = Colors[((y & 1) + x) & 1];
            SDL_SetRenderDrawColor(device, color, color, color, 255);
            SDL_RenderFillRect(device, &dst);
        }
    }
}

static void DrawErrrorDialogs(Project *project) {
    DrawMessageDialog(Error_InvalidImage, ErrorM_InvalidImage,
                      project->error_msg.c_str());

    if (project->error_id != nullptr) {
        ImGui::OpenPopup(project->error_id);
        project->error_id = nullptr;
    }
}

static void MouseDrag(const std::unique_ptr<Atlas> &atlas) {
    int x, y;
    auto mouse = SDL_GetMouseState(&x, &y);

    if (SDL_BUTTON(3) & mouse || SDL_BUTTON(2) & mouse) {
        atlas->position.x = (x - atlas->origin.x);
        atlas->position.y = (y - atlas->origin.y);
    }
}

void RenderUi(SDL_Renderer *device, Project *project) {
    auto &io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 320, 20));
    ImGui::SetNextWindowSize(ImVec2(300, 400));
    DrawAtlasWindow(*project);

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 320, 440));
    ImGui::SetNextWindowSize(ImVec2(300, (io.DisplaySize.y - 400) - 60));
    DrawProjectWindow(device, project);

    ImGui::SetNextWindowPos(ImVec2(20, 20));
    ImGui::SetNextWindowSize(ImVec2(300, 300));
    DrawAnimationsWindow(*project);

    ImGui::SetNextWindowPos(ImVec2(20, 340));
    ImGui::SetNextWindowSize(ImVec2(300, (io.DisplaySize.y - 300) - 60));
    DrawSpritesWindow(*project);

    DrawErrrorDialogs(project);

    auto &atlas = project->GetAtlas();
    if (atlas->sprites.size() == 0) {
        DrawNoSpritesInfo();
        return;
    }
    MouseDrag(atlas);

    float ox = atlas->position.x;
    float oy = atlas->position.y;
    float scale = exp(atlas->scale - 1.0f);
    ox += io.DisplaySize.x * 0.5f - float(atlas->width) * scale * 0.5f;
    oy += io.DisplaySize.y * 0.5f - float(atlas->height) * scale * 0.5f;

    SDL_Rect dst{int(ox), int(oy),
                 int(atlas->width * scale),
                 int(atlas->height * scale)};

	DrawBackground(device, dst);
    SDL_Rect border = {dst.x - 1, dst.y - 1, dst.w + 2, dst.h + 2};
    SDL_SetRenderDrawColor(device, 0, 0, 0, 255);
    SDL_RenderDrawRect(device, &border);

    assert(atlas->texture != nullptr);
    SDL_RenderCopy(device, atlas->texture, nullptr, &dst);
}

void ProcessEvent(SDL_Renderer *device, Project *project, const SDL_Event &e) {
    auto &io = ImGui::GetIO();
    auto &atlas = project->GetAtlas();
    switch (e.type) {
    case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            io.DisplaySize.x = float(e.window.data1);
            io.DisplaySize.y = float(e.window.data2);
        }
        break;
    case SDL_DROPFILE:
        if (HasExtension(e.drop.file, ".spritepack")) {
            project->Load(device, e.drop.file);
            SDL_free(e.drop.file);
            break;
        }
        if (atlas->AppendSprite(e.drop.file, atlas->selected_anim))
            atlas->Render();
        else
            project->Error(Error_InvalidImage, e.drop.file);
        SDL_free(e.drop.file);
        break;
    case SDL_KEYDOWN:
        switch (e.key.keysym.sym) {
        case SDLK_SPACE:
            // Reset transform
            atlas->position = SDL_Point{0, 0};
            atlas->origin = SDL_Point{0, 0};
            atlas->scale = 1.0f;
            break;
        case SDLK_s:
            if (e.key.keysym.mod & KMOD_CTRL)
                project->Save();
            break;
        case SDLK_e:
            if (e.key.keysym.mod & KMOD_CTRL)
                atlas->Export(project->exporters[atlas->exporter].second);
            break;
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == 3 || e.button.button == 2) {
            atlas->origin.x = e.button.x - atlas->position.x;
            atlas->origin.y = e.button.y - atlas->position.y;
        }
        break;
    case SDL_MOUSEWHEEL:
        {
            int dir = e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
            if (!io.WantCaptureMouse)
                atlas->SetZoom(dir * e.wheel.y);
            io.MouseWheel = float(e.wheel.y);
        }
        break;
    }
}

void MainLoop(SDL_Renderer *device, Project *project) {
    assert(device != nullptr && project != nullptr);
    auto frame_begin = std::chrono::high_resolution_clock::now();
    auto frame_end = frame_begin;

    auto &io = ImGui::GetIO();
    SDL_Event e;
    for (;;) {
        frame_begin = frame_end;
        frame_end = std::chrono::high_resolution_clock::now();
        auto dt_micro = std::chrono::duration_cast<std::chrono::microseconds>(
            (frame_end - frame_begin)).count();
        float dt = float(double(dt_micro) * 1e-6);

        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            ProcessEvent(device, project, e);
            if (e.type == SDL_QUIT) {
                return;
            }
        }
        int mx, my;
        int button = SDL_GetMouseState(&mx, &my);
        io.DeltaTime = dt;
        io.MousePos = ImVec2(float(mx), float(my));
        io.MouseDown[0] = button & SDL_BUTTON(SDL_BUTTON_LEFT);
        io.MouseDown[1] = button & SDL_BUTTON(SDL_BUTTON_RIGHT);

        SDL_SetRenderTarget(device, nullptr);
        SDL_SetRenderDrawColor(device, 0x12, 0x12, 0x21, 0xff);
        SDL_RenderClear(device);
#ifdef WIN32
        // Screen doesn't clear properly with Direct3D for some reason.
        // Might be because of the ImGUI SDL layer.
        SDL_RenderFillRect(device, nullptr);
#endif
        ImGui::NewFrame();
        RenderUi(device, project);
        ImGui::Render();
        ImGuiSDL::Render(ImGui::GetDrawData());
        SDL_RenderPresent(device);
    }
}

} // namespace spack
