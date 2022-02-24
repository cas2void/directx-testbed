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

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using SecondsAsFloat = std::chrono::duration<float>;

void SketchBase::Reset()
{
    startTime_ = std::chrono::high_resolution_clock::now();
    previousTime_ = startTime_;
    deltaTime_ = 0.0f;
    elapsedTime_ = 0.0f;
}

void SketchBase::Tick()
{
    Statistics();

    high_resolution_clock::time_point currentTime = high_resolution_clock::now();
    deltaTime_ = duration_cast<SecondsAsFloat>(currentTime - previousTime_).count();
    elapsedTime_ = duration_cast<SecondsAsFloat>(currentTime - startTime_).count();
    previousTime_ = currentTime;
}

float SketchBase::GetDeltaTime() const
{
    return deltaTime_;
}

float SketchBase::GetElapsedTime() const
{
    return elapsedTime_;
}

float SketchBase::GetAverageFrameTime() const
{
    return averageFrameTime_;
}

float SketchBase::GetAverageFPS() const
{
    if (averageFrameTime_ <= 0.0f)
    {
        return 0.0f;
    }
    else
    {
        return 1.0f / averageFrameTime_;
    }
}

void SketchBase::Statistics()
{
    static int numFrames = 0;
    static high_resolution_clock::time_point previousStatisticsTime = high_resolution_clock::now();

    numFrames++;

    high_resolution_clock::time_point currentTime = high_resolution_clock::now();
    float interval = duration_cast<SecondsAsFloat>(currentTime - previousStatisticsTime).count();
    if (interval > 1.0f)
    {
        averageFrameTime_ = interval / numFrames;

        numFrames = 0;
        previousStatisticsTime = currentTime;
    }
}

}; // namepspace sketch
