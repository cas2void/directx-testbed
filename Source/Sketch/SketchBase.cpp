#include "SketchBase.h"

namespace sketch
{

void SketchBase::Configurate(std::function<void(Config&)> configurator)
{
    configurator(config_);
}

const SketchBase::Config& SketchBase::GetConfig() const
{
    return config_;
}

}; // namepspace sketch
