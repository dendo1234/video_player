#include <SDL3/SDL.h>
#define CLAY_IMPLEMENTATION
#include <clay.h>
#include <memory>

void HandleClayErrors(Clay_ErrorData errorData) {
    // See the Clay_ErrorData struct for more information
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", errorData.errorText.chars);
    switch(errorData.errorType) {
        // etc
    }
}

char* ClayInit() {
    uint64_t totalMemorySize = Clay_MinMemorySize();
    std::allocator<char> allocator;
    auto ptr = allocator.allocate(totalMemorySize);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, ptr);

    // Note: screenWidth and screenHeight will need to come from your environment, Clay doesn't handle window related tasks
    Clay_Initialize(arena, (Clay_Dimensions) { 640, 360 }, (Clay_ErrorHandler) { HandleClayErrors });

    return ptr;
}

void ClayDestructor(char* ptr) {
    uint64_t totalMemorySize = Clay_MinMemorySize();

    std::allocator<char> allocator;
    allocator.deallocate(ptr, totalMemorySize);
}