#include "SketchBase.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using SecondsAsFloat = std::chrono::duration<float>;

namespace sketch
{

void SketchBase::SetConfig(std::function<void(Config&)> configSetter)
{
    configSetter(config_);
}

const SketchBase::Config& SketchBase::GetConfig() const
{
    return config_;
}

void SketchBase::SetFeature(std::function<void(Feature&)> featureSetter)
{
    featureSetter(feature_);
}

const SketchBase::Feature& SketchBase::GetFeature() const
{
    return feature_;
}

const SketchBase::State& SketchBase::GetState() const
{
    return state_;
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

void SketchBase::Init()
{
    OnInit();
}

void SketchBase::Update()
{
    OnUpdate();
}

void SketchBase::Quit()
{
    OnQuit();
}

void SketchBase::Resize(int width, int height)
{
    static int previousWidth = 0;
    static int previousHeight = 0;

    if (width != previousWidth || height != previousHeight)
    {
        previousWidth = width;
        previousHeight = height;

        state_.ViewportWidth = width;
        state_.ViewportHeight = height;
        OnResize(width, height);
    }
}

void SketchBase::MouseDown(int x, int y, MouseButtonType buttonType)
{
    OnMouseDown(x, y, buttonType);
}

void SketchBase::MouseUp(int x, int y, MouseButtonType buttonType)
{
    OnMouseUp(x, y, buttonType);
}

void SketchBase::MouseDrag(int x, int y, MouseButtonType buttonType)
{
    OnMouseDrag(x, y, buttonType);
}

void SketchBase::MouseMove(int x, int y)
{
    // Prevent noise
    static int previousX = -1;
    static int previousY = -1;
    if (x != previousX || y != previousY)
    {
        OnMouseMove(x, y);
        previousX = x;
        previousY = y;
    }
}

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

void SketchBase::Pause()
{

}

void SketchBase::Resume()
{

}

}; // namepspace sketch
