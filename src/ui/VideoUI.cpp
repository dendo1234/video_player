#include <ui/VideoUI.hpp>
#include <clay.h>
#include <clay_renderer_SDL3.h>
#include <ui/layout.h>

// Please note: -Wmissing-field-initializers is disabled for this file for making it less boilerplate to the the UI.

const Clay_Color COLOR_BLACK = Clay_Color{0, 0, 0, 255};
const Clay_Color COLOR_WHITE = Clay_Color{255, 255, 255, 255};
const Clay_Color COLOR_LIGHT = Clay_Color{224, 215, 210, 255};
const Clay_Color COLOR_RED = Clay_Color{168, 66, 28, 255};
const Clay_Color COLOR_GREEN = Clay_Color{0, 255, 0, 255};
const Clay_Color COLOR_ORANGE = Clay_Color{225, 138, 50, 255};
const Clay_Color COLOR_TRANSPARENT = Clay_Color{0, 0, 0, 0};

VideoUI::VideoUI(const Layer& layer, Video* video)
    : Layer{layer},
      video{video} {

}

void VideoUI::OnRender() {
    int screenWidth;
    int screenHeight;
    windowTarget->GetWindowSize(screenWidth,screenHeight);

    float mousePositionX;
    float mousePositionY;
    windowTarget->GetMousePosition(mousePositionX, mousePositionY);

    // Optional: Update internal layout dimensions to support resizing
    Clay_Dimensions layoutDim = {
        .width = static_cast<float>(screenWidth),
        .height = static_cast<float>(screenHeight)
    };
    Clay_SetLayoutDimensions(layoutDim);
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling & debug tools
    Clay_Vector2 mousePos = {
        .x = mousePositionX,
        .y = mousePositionY
    };
    Clay_SetPointerState(mousePos, false);
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling and debug tools
    // Clay_UpdateScrollContainers(true, (Clay_Vector2) { mouseWheelX, mouseWheelY }, deltaTime);

    // Clay_LayoutConfig rootLayout = Clay_LayoutConfig { 
    //     .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
    //     .childAlignment = {
    //         .y = Clay_LayoutAlignmentY::CLAY_ALIGN_Y_BOTTOM
    //     },
    // };

    Clay_RenderCommandArray renderCommands;

    layout(&renderCommands, progressPercentage);

    Clay_SDL3RendererData data = {
        .renderer = windowTarget->GetRenderer(),
    };

    SDL_SetRenderTarget(windowTarget->GetRenderer(), nullptr);
    SDL_SetRenderLogicalPresentation(windowTarget->GetRenderer(), 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_Clay_RenderClayCommands(&data, &renderCommands);

}

void VideoUI::OnUpdate(double dt) {
    auto _ = dt;

    progressPercentage = video->GetPlaybackPercentage();
}

EventResult VideoUI::OnEvent(const Event& event) {
    auto _ = event;
    return EventResult::Continue;

}
