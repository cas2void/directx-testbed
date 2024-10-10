#include "sketch-base.h"
#include "launcher.h"

class DummySketch final : public engine::SketchBase
{
};

LAUNCH_SKETCH(DummySketch)
