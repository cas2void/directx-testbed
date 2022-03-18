#pragma once

#include <functional>
#include <chrono>

namespace sketch
{

class SketchBase
{
public:
    virtual void OnInit() {}
    virtual void OnUpdate() {}
    virtual void OnQuit() {}
    virtual void OnResize(int width, int height) { (void)width; (void)height; }

    //
    // Config
    //
public:
    struct Config
    {
        int width = 960;
        int height = 540;
        bool vsync = true;
    };

    void Configurate(std::function<void(Config&)> configurator);
    const Config& GetConfig() const;

private:
    Config config_;

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

    void Reset();
    void Tick();
    void Pause();
    void Resume();
};

}; // namespace sketch