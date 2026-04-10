#include <core/Layer.hpp>
#include <video/Video.hpp>
#include <clay.h>

class VideoUI : public Layer {
private:
    float progressPercentage{0};
    Video& video;
    Clay_RenderCommandArray BuildLayout() const;
    bool mouse1Pressed{false};
    TTF_Font** fonts;
    std::string playbackProgressText;
public:
    VideoUI(const Layer& layer, Video& video, TTF_Font** fonts);

    void OnRender() override;
    void OnUpdate(double dt) override;
    EventResult OnEvent(const Event& event) override;

    void SetProgressPercentage(float percentage);

    friend void ProgressBarClick(Clay_ElementId elementId, Clay_PointerData pointerData, intptr_t userData);
};

