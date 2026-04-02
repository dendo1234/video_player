#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <clay.h>
#include <memory>

void HandleClayErrors(Clay_ErrorData errorData) {
    // See the Clay_ErrorData struct for more information
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", errorData.errorText.chars);
    switch(errorData.errorType) {
        // etc
        default:
        break;
    }
}

static inline Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void *userData)
{
    TTF_Font **fonts = reinterpret_cast<TTF_Font**>(userData);
    TTF_Font *font = fonts[config->fontId];
    int width, height;

    TTF_SetFontSize(font, config->fontSize);
    if (!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
    }

    return Clay_Dimensions{ (float) width, (float) height };
}

char* ClayInit() {
    uint64_t totalMemorySize = Clay_MinMemorySize();
    std::allocator<char> allocator;
    auto ptr = allocator.allocate(totalMemorySize);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, ptr);

    // Note: screenWidth and screenHeight will need to come from your environment, Clay doesn't handle window related tasks
    Clay_ErrorHandler clay_eh = {
        .errorHandlerFunction = HandleClayErrors,
        .userData = nullptr
    };
    Clay_Dimensions dim = {
        .width = 640,
        .height = 360
    };
    Clay_Initialize(arena, dim, clay_eh);

    Clay_SetMeasureTextFunction(SDL_MeasureText, nullptr);

    return (char*)ptr;
}

void ClayDestructor(char* ptr) {
    uint64_t totalMemorySize = Clay_MinMemorySize();

    std::allocator<char> allocator;
    allocator.deallocate(ptr, totalMemorySize);
}