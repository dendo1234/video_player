#include <ui/VideoUI.hpp>
#include <clay.h>


void VideoUI::OnRender() {

}

void VideoUI::OnUpdate(double dt) {

    int screenWidth;
    int screenHeight;
    windowTarget->GetWindowSize(screenWidth,screenHeight);

    float mousePositionX;
    float mousePositionY;
    windowTarget->GetMousePosition(mousePositionX, mousePositionY);

    // Optional: Update internal layout dimensions to support resizing
    Clay_SetLayoutDimensions((Clay_Dimensions) { screenWidth, screenHeight });
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling & debug tools
    Clay_SetPointerState((Clay_Vector2) { mousePositionX, mousePositionY }, false);
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling and debug tools
    // Clay_UpdateScrollContainers(true, (Clay_Vector2) { mouseWheelX, mouseWheelY }, deltaTime);

    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    Clay_BeginLayout();

    // An example of laying out a UI with a fixed width sidebar and flexible width main content
    CLAY(CLAY_ID("OuterContainer"), { .layout = { .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(16), .childGap = 16 }, .backgroundColor = {250,250,255,255} }) {
        CLAY(CLAY_ID("SideBar"), {
            .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = { .width = CLAY_SIZING_FIXED(300), .height = CLAY_SIZING_GROW(0) }, .padding = CLAY_PADDING_ALL(16), .childGap = 16 },
            .backgroundColor = COLOR_LIGHT
        }) {
            CLAY(CLAY_ID("ProfilePictureOuter"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) }, .padding = CLAY_PADDING_ALL(16), .childGap = 16, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } }, .backgroundColor = COLOR_RED }) {
                CLAY(CLAY_ID("ProfilePicture"), {.layout = { .sizing = { .width = CLAY_SIZING_FIXED(60), .height = CLAY_SIZING_FIXED(60) }}, .image = { .imageData = &profilePicture } }) {}
                CLAY_TEXT(CLAY_STRING("Clay - UI Library"), CLAY_TEXT_CONFIG({ .fontSize = 24, .textColor = {255, 255, 255, 255} }));
            }


            CLAY(CLAY_ID("MainContent"), { .layout = { .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) } }, .backgroundColor = COLOR_LIGHT }) {}
        }
    }

    // All clay layouts are declared between Clay_BeginLayout and Clay_EndLayout
    Clay_RenderCommandArray renderCommands = Clay_EndLayout();

    // More comprehensive rendering examples can be found in the renderers/ directory
    for (int i = 0; i < renderCommands.length; i++) {
        Clay_RenderCommand *renderCommand = &renderCommands.internalArray[i];

        switch (renderCommand->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                DrawRectangle( renderCommand->boundingBox, renderCommand->renderData.rectangle.backgroundColor);
            }
            // ... Implement handling of other command types
        }
    }
}

EventResult VideoUI::OnEvent(const Event& event) {

}
