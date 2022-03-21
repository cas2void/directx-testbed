#pragma once

#include <functional>
#include <chrono>

namespace sketch
{

enum class MouseButtonType
{
    kLeft,
    kRight
};

class SketchBase
{
public:
    virtual void OnInit() {}
    virtual void OnUpdate() {}
    virtual void OnQuit() {}
    virtual void OnResize(int width, int height) { (void)width; (void)height; }
    virtual void OnMouseDown(int x, int y, MouseButtonType buttonType) { (void)x; (void)y; (void)buttonType; }
    virtual void OnMouseUp(int x, int y, MouseButtonType buttonType) { (void)x; (void)y; (void)buttonType; }
    virtual void OnMouseDrag(int x, int y, MouseButtonType buttonType) { (void)x; (void)y; (void)buttonType; }
    virtual void OnMouseMove(int x, int y) { (void)x; (void)y; }

    //
    // Config
    //
public:
    struct Config
    {
        int X = 480;
        int Y = 270;
        int Width = 960;
        int Height = 540;
        bool Vsync = true;
        bool WindowModeSwitch = false;
        bool Fullscreen = false;
    };

    void SetConfig(std::function<void(Config&)> configSetter);
    const Config& GetConfig() const;

private:
    Config config_;

    //
    // Feature
    //
public:
    struct Feature
    {
        bool Tearing = false;
    };

    void SetFeature(std::function<void(Feature&)> featureSetter);
    const Feature& GetFeature() const;

protected:
    Feature feature_;

    //
    // State
    //
public:
    struct State
    {
        int ViewportWidth;
        int ViewportHeight;
    };

    const State& GetState() const;

private:
    State state_;

    //
    // Timing
    //
public:
    //
    // Framework interfaces, do not call these in apps.
    //

    // dt in seconds
    float GetDeltaTime() const;
    float GetElapsedTime() const;

private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::chrono::high_resolution_clock::time_point previousTime_;
    float deltaTime_;
    float elapsedTime_;

    //
    // Statistics
    //
public:
    float GetAverageFrameTime() const;
    float GetAverageFPS() const;

private:
    void Statistics();
    float averageFrameTime_;

    //
    // Framework interfaces, do not call these in apps.
    //
public:
    void Init();
    void Update();
    void Quit();
    void Resize(int width, int height);

    void MouseDown(int x, int y, MouseButtonType buttonType);
    void MouseUp(int x, int y, MouseButtonType buttonType);
    void MouseDrag(int x, int y, MouseButtonType buttonType);
    void MouseMove(int x, int y);

    void Reset();
    void Tick();
    void Pause();
    void Resume();
};

}; // namespace sketch