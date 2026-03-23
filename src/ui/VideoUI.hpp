#include <core/Layer.hpp>

class VideoUI : public Layer {

    void OnRender() override;
    void OnUpdate(double dt) override;
    EventResult OnEvent(const Event& event) override;

};

