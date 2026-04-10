#include <core/FontManager.hpp>


FontManager::FontManager() {
    for (size_t i = 0; i < fontPaths.size(); i++) {
        fonts[i] = TTF_OpenFont(fontPaths[i], 1);
        if (fonts[i] == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open font file %s: %s", fontPaths[i], SDL_GetError());
        }
    }
}

FontManager::~FontManager() {
    for (auto &&font : fonts)
    {
        TTF_CloseFont(font);
    }
}