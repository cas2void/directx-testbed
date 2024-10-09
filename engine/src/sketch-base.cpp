#include "sketch-base.h"

using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using SecondsAsDouble = std::chrono::duration<double>;

namespace engine
{
SketchBase::SketchBase()
    : delta_time_(0), elapsed_time_(0), average_frame_time_(0)
{}

const SketchBase::Config& SketchBase::GetConfig() const
{
    return config_;
}

const SketchBase::Feature& SketchBase::GetFeature() const
{
    return feature_;
}

const SketchBase::State& SketchBase::GetState() const
{
    return state_;
}

double SketchBase::GetDeltaTime() const
{
    return delta_time_;
}

double SketchBase::GetElapsedTime() const
{
    return elapsed_time_;
}

double SketchBase::GetAverageFrameTime() const
{
    return average_frame_time_;
}

double SketchBase::GetAverageFps() const
{
    if (average_frame_time_ > 0)
    {
        return 1.0 / average_frame_time_;
    }
    return 0.0;
}

void SketchBase::Statistics()
{
    static auto num_frames = 0;
    static auto previous_statistics_time = high_resolution_clock::now();

    num_frames++;

    const auto current_time = high_resolution_clock::now();
    if (const auto interval = duration_cast<SecondsAsDouble>(current_time - previous_statistics_time).count();
        interval > 1.0)
    {
        average_frame_time_ = interval / num_frames;

        num_frames = 0;
        previous_statistics_time = current_time;
    }
}

void SketchBase::Configure()
{
    OnConfigure(config_);
}

void SketchBase::Init()
{
    OnInit();
}

void SketchBase::Reset()
{
    start_time_ = high_resolution_clock::now();
    previous_time_ = start_time_;
    delta_time_ = 0.0;
    elapsed_time_ = 0.0;
}

void SketchBase::Tick()
{
    OnTick();

    Statistics();

    const auto current_time = high_resolution_clock::now();
    delta_time_ = duration_cast<SecondsAsDouble>(current_time - previous_time_).count();
    elapsed_time_ = duration_cast<SecondsAsDouble>(current_time - start_time_).count();
    previous_time_ = current_time;
}

void SketchBase::Quit()
{
    OnQuit();
}

void SketchBase::Resize(const int width, const int height)
{
    static int previous_width = 0;
    static int previous_height = 0;

    if (width != previous_width || height != previous_height)
    {
        previous_width = width;
        previous_height = height;

        state_.viewport_width = width;
        state_.viewport_height = height;
        OnResize(width, height);
    }
}

void SketchBase::MouseDown(const int x, const int y, const MouseButtonType button_type)
{
    OnMouseDown(x, y, button_type);
}

void SketchBase::MouseUp(const int x, const int y, const MouseButtonType button_type)
{
    OnMouseUp(x, y, button_type);
}

void SketchBase::MouseDrag(const int x, const int y, const MouseButtonType button_type)
{
    OnMouseDrag(x, y, button_type);
}

void SketchBase::MouseMove(const int x, const int y)
{
    // Prevent noise
    static int previous_x = -1;
    static int previous_y = -1;
    if (x != previous_x || y != previous_y)
    {
        OnMouseMove(x, y);
        previous_x = x;
        previous_y = y;
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void SketchBase::Pause()
{}

// ReSharper disable once CppMemberFunctionMayBeStatic
void SketchBase::Resume()
{}
}; // namespace sketch
