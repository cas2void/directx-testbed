#include "SketchBase.h"
#include "Launcher.h"

class Ch04_Initialization : public sketch::SketchBase
{

};

CREATE_SKETCH(Ch04_Initialization,
    [](sketch::SketchConfig& config)
    {
        config.width = 1280;
        config.height = 720; 
    }
)