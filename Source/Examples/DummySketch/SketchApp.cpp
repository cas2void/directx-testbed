#include "Launcher.h"

class DummySketch : public sketch::SketchBase
{

};

CREATE_SKETCH(DummySketch, 
    [](sketch::SketchBase::Config& config)
    {
        config.width = 1280;
        config.height = 720; 
    }
)