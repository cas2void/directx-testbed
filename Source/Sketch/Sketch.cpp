#include "Sketch.h"

namespace sketch
{

void SketchBase::Init()
{
}

void SketchBase::Update(float dt)
{
    (void)dt;
}

void SketchBase::Quit()
{
}

void SketchBase::Configurate(std::function<void(sketch::SketchConfig&)> configurator)
{
    configurator(config_);
}

SketchConfig SketchBase::GetConfig() const
{
    return config_;
}

}; // namepspace sketch
