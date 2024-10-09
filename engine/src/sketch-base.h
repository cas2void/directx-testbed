#pragma once

#include <chrono>

namespace engine
{
enum class MouseButtonType
{
    kLeft,
    kRight
};

class SketchBase
{
public:
    SketchBase();
    virtual ~SketchBase() = default;

    SketchBase(const SketchBase&) = delete;
    SketchBase(SketchBase&&) = delete;
    SketchBase& operator=(const SketchBase&) = delete;
    SketchBase& operator=(SketchBase&&) = delete;

#pragma region SketchBaseInterfaces
public:
    struct Config;
    
protected:
    virtual void OnConfigure(Config& /*config*/)
    {}
    
    virtual void OnInit()
    {}

    virtual void OnTick()
    {}

    virtual void OnQuit()
    {}

    virtual void OnResize(int /*width*/, int /*height*/)
    {}

    virtual void OnMouseDown(int /*x*/, int /*y*/, MouseButtonType /*button_type*/)
    {}

    virtual void OnMouseUp(int /*x*/, int /*y*/, MouseButtonType /*button_type*/)
    {}

    virtual void OnMouseDrag(int /*x*/, int /*y*/, MouseButtonType /*button_type*/)
    {}

    virtual void OnMouseMove(int /*x*/, int /*y*/)
    {}
#pragma endregion

#pragma region Config
public:
    struct Config
    {
        int x = -1;
        int y = -1;
        int width = 0;
        int height = 0;
        bool vsync = true;
        bool window_mode_switch = false;
        bool fullscreen = false;
    };

    [[nodiscard]] const Config& GetConfig() const;

private:
    Config config_;
#pragma endregion

#pragma region Feature
public:
    struct Feature
    {
        bool tearing = false;
    };

    [[nodiscard]] const Feature& GetFeature() const;

private:
    Feature feature_;
#pragma endregion

#pragma region State
public:
    struct State
    {
        int viewport_width = 0;
        int viewport_height = 0;
    };

    [[nodiscard]] const State& GetState() const;

private:
    State state_;
#pragma endregion

#pragma region Timing
public:
    // dt in seconds
    [[nodiscard]] double GetDeltaTime() const;
    [[nodiscard]] double GetElapsedTime() const;

private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::chrono::high_resolution_clock::time_point previous_time_;
    double delta_time_;
    double elapsed_time_;
#pragma endregion

#pragma region Statistics
public:
    [[nodiscard]] double GetAverageFrameTime() const;
    [[nodiscard]] double GetAverageFps() const;

private:
    void Statistics();
    double average_frame_time_;
#pragma endregion

#pragma region FrameworkInterfaces
private:
    // Framework interfaces, hide from call in apps.
    void Configure();
    void Init();
    void Reset();
    void Tick();
    void Quit();
    
    void Resize(int width, int height);
    void MouseDown(int x, int y, MouseButtonType button_type);
    void MouseUp(int x, int y, MouseButtonType button_type);
    void MouseDrag(int x, int y, MouseButtonType button_type);
    void MouseMove(int x, int y);

    void Pause();
    void Resume();

    friend class Launcher;
#pragma endregion
};
}; // namespace sketch
