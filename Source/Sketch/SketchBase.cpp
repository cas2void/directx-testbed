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

void SketchBase::Reset()
{
    startTime_ = std::chrono::high_resolution_clock::now();
    previousTime_ = startTime_;
}

using Duration = std::chrono::duration<float, std::milli>;

void SketchBase::Tick()
{
    std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
    Duration interval = currentTime - previousTime_;
    deltaTime_ = interval.count() * Duration::period::num / Duration::period::den;
    previousTime_ = currentTime;
}

}; // namepspace sketch
