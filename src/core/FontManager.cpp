#include <core/FontManager.hpp>
#include <DejaVuSans.h>


FontManager::FontManager() {
    SDL_IOStream* buf = SDL_IOFromConstMem(__DejaVuSans_ttf, __DejaVuSans_ttf_len);
    TTF_Font* font = TTF_OpenFontIO(buf, true, 1);
    fonts[0] = font;
    if (fonts[0] == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open font file %s: %s", fontPaths[0], SDL_GetError());
    }
}

FontManager::~FontManager() {
    for (auto &&font : fonts)
    {
        TTF_CloseFont(font);
    }
}