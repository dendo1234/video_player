#pragma once
#include <sdl3_ttf/SDL_ttf.h>
#include <array>
#include <string>
#include <memory>

constexpr auto fontPaths{std::to_array({
    "DejaVuSans.ttf"
})};


class FontManager {
    std::array<TTF_Font*,fontPaths.size()> fonts;

public:
    FontManager();
    ~FontManager();

    TTF_Font** GetFonts() {
        return fonts.begin();
    }
};