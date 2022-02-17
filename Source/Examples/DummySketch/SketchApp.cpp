#include "SketchBase.h"
#include "Launcher.h"

class DummySketch : public sketch::SketchBase
{

};

CREATE_SKETCH(DummySketch, 
    [](sketch::SketchConfig& config)
    {
        config.width = 1280;
        config.height = 720; 
    }
)