#include <core/Layer.hpp>
#include <video/Video.hpp>



class VideoUI : public Layer {
private:
    float progressPercentage{0};
    Video* video{nullptr};

public:
    VideoUI(const Layer& layer, Video* video);

    void OnRender() override;
    void OnUpdate(double dt) override;
    EventResult OnEvent(const Event& event) override;

    void SetProgressPercentage(float percentage);

};

