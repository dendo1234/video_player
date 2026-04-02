#include <clay.h>

// unfortunetaly the layout has to be in a C file to be able to be compiled by MSVC. 
// It will fill with garbage unitialized fields in C++ files, while we need to be filled with zeros

const Clay_Color COLOR_BLACK = {0, 0, 0, 255};
const Clay_Color COLOR_WHITE = {255, 255, 255, 255};
const Clay_Color COLOR_LIGHT = {224, 215, 210, 255};
const Clay_Color COLOR_RED = {168, 66, 28, 255};
const Clay_Color COLOR_GREEN = {0, 255, 0, 255};
const Clay_Color COLOR_ORANGE = {225, 138, 50, 255};
const Clay_Color COLOR_TRANSPARENT = {0, 0, 0, 0};

void layout(Clay_RenderCommandArray* renderCommandsBuffer, float progressPercentage) {
    Clay_LayoutConfig rootLayout = { 
        .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
        .childAlignment = {
            .y = CLAY_ALIGN_Y_BOTTOM
        },
    };

    Clay_BeginLayout(); 
    CLAY({ .layout = rootLayout, .backgroundColor = {255,255,255,0} }) {
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
            CLAY({
                .layout = {
                    .sizing = { .width = CLAY_SIZING_PERCENT(progressPercentage), .height = CLAY_SIZING_GROW() }
                },
                .backgroundColor = COLOR_GREEN,
            }) {}
        }
    }
    *renderCommandsBuffer = Clay_EndLayout();
}