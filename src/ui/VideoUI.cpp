#include <ui/VideoUI.hpp>
#include <clay.h>
#include <clay_renderer_SDL3.h>
#include <string>
#include <format>

// Please note: -Wmissing-field-initializers is disabled for this file for making it less boilerplate to the the UI.

constexpr Clay_Color COLOR_BLACK = Clay_Color{0, 0, 0, 255};
constexpr Clay_Color COLOR_WHITE = Clay_Color{255, 255, 255, 255};
// constexpr Clay_Color COLOR_LIGHT = Clay_Color{224, 215, 210, 255};
// constexpr Clay_Color COLOR_RED = Clay_Color{168, 66, 28, 255};
constexpr Clay_Color COLOR_GREEN = Clay_Color{0, 255, 0, 255};
// constexpr Clay_Color COLOR_ORANGE = Clay_Color{225, 138, 50, 255};
// constexpr Clay_Color COLOR_TRANSPARENT = Clay_Color{0, 0, 0, 0};

void ProgressBarClick(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData) {
    VideoUI* videoUI = (VideoUI*)userData;
    if (pointerData.state == CLAY_POINTER_DATA_PRESSED) {
        int w, h;
        videoUI->windowTarget->GetWindowSize(w, h);

        Clay_ElementData elementData = Clay_GetElementData(elementId);
        SDL_assert(elementData.found);
        double percentage = static_cast<double>(pointerData.position.x-elementData.boundingBox.x)/elementData.boundingBox.width;

        double timestamp = videoUI->video.ConvertPercentageToTimestamp(percentage);
        
        videoUI->video.RequestSeek(timestamp);
    }
}

Clay_RenderCommandArray VideoUI::BuildLayout() const {
    Clay_LayoutConfig rootLayout = { 
        .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
        .childAlignment = {
            .y = CLAY_ALIGN_Y_BOTTOM
        },
    };

    Clay_TextElementConfig textConfig = {
        .textColor = COLOR_GREEN,
        .fontId = 0,
        .fontSize = 10,
        .lineHeight = 0
    };

    Clay_BeginLayout(); 
    CLAY({ .layout = rootLayout, .backgroundColor = {255,255,255,0} }) {
        CLAY({
            .id = CLAY_ID("Bottom UI"),
            .layout = {
                .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            .backgroundColor = COLOR_WHITE
        }) {
            CLAY({
                .id = CLAY_ID("Top part"),
                .layout = {
                    .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
                    .padding = CLAY_PADDING_ALL(4),
                },
            }) {
                CLAY({
                    .id = CLAY_ID("Playback progress bar"),
                    .layout = {
                        .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(18) },
                    },
                    .backgroundColor = COLOR_WHITE,
                    .border = {
                        .color = COLOR_BLACK,
                        .width = CLAY_BORDER_ALL(1)
                    }
                }) {
                    Clay_OnHover(ProgressBarClick, (intptr_t)this);
                    CLAY({
                        .layout = {
                            .sizing = { .width = CLAY_SIZING_PERCENT(progressPercentage), .height = CLAY_SIZING_GROW() }
                        },
                        .backgroundColor = COLOR_GREEN,
                    }) {}
                }
            }
            CLAY({
                .id = CLAY_ID("Bottom info bar"),
                .layout = {
                    .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }
                },
                .backgroundColor = COLOR_BLACK,
            }) {
                Clay_String text = {
                    .isStaticallyAllocated = false,
                    .length = static_cast<int>(playbackProgressText.length()),
                    .chars = playbackProgressText.data()
                };
                CLAY_TEXT(text, &textConfig);

            }

        }
    }

    return Clay_EndLayout();
}

VideoUI::VideoUI(const Layer& layer, Video& video, TTF_Font** fonts)
    : Layer{layer},
      video{video},
      fonts{fonts} {

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
    Clay_SetPointerState(mousePos, mouse1Pressed);
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling and debug tools
    // Clay_UpdateScrollContainers(true, (Clay_Vector2) { mouseWheelX, mouseWheelY }, deltaTime);

    Clay_RenderCommandArray renderCommands = BuildLayout();

    Clay_SDL3RendererData data = {
        .renderer = windowTarget->GetRenderer(),
        .textEngine = windowTarget->GetTextEngine(),
        .fonts = fonts,
    };

    SDL_SetRenderTarget(windowTarget->GetRenderer(), nullptr);
    SDL_SetRenderLogicalPresentation(windowTarget->GetRenderer(), 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_Clay_RenderClayCommands(&data, &renderCommands);
    // Should we re-enable logical presentation? 

}

void VideoUI::OnUpdate(double dt) {
    auto _ = dt;

    progressPercentage = SDL_clamp(video.GetPlaybackPercentage(), 0.0, 1.0);

    uint64_t totalSeconds = video.GetMasterClock();
    uint64_t hours = totalSeconds/3600;
    uint64_t minutes = (totalSeconds%3600)/60;
    double seconds = SDL_fmod(video.GetMasterClock(), 60);

    uint64_t durationTotalSeconds = video.GetDuration();
    uint64_t durationHours = durationTotalSeconds/3600;
    uint64_t durationMinutes = (durationTotalSeconds%3600)/60;
    double durationSeconds = SDL_fmod(video.GetDuration(), 60);


    playbackProgressText = std::format("{:02}:{:02}:{:02.2f} / {:02}:{:02}:{:02.2f}", hours, minutes, seconds, durationHours, durationMinutes, durationSeconds);
}

EventResult VideoUI::OnEvent(const Event& event) {
    switch (event.sdl_event->type)
    {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event.sdl_event->button.button == 1) {
            mouse1Pressed = true;
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.sdl_event->button.button == 1) {
            mouse1Pressed = false;
        }
        break;  
    default:
        break;
    }
    return EventResult::Continue;
}
