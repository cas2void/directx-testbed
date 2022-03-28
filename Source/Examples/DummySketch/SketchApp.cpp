#include "Launcher.h"

class DummySketch : public sketch::SketchBase
{

};

CREATE_SKETCH(DummySketch, 
    [](sketch::SketchBase::Config& config)
    {
        config.Width = 1280;
        config.Height = 720; 
    }
)