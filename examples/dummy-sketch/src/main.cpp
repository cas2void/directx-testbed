#include "sketch-base.h"
#include "launcher.h"

class DummySketch final : public engine::SketchBase
{
    void OnConfigure(Config& config) override
    {
        config.x = 100;
        config.y = 100;
        config.width = 1280;
        config.height = 720;
    }
};

LAUNCH_SKETCH(DummySketch)
