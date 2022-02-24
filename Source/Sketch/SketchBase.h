#pragma once

#include <functional>
#include <chrono>

namespace sketch
{

class SketchBase
{
public:
    virtual void Init() {}
    virtual void Update() {}
    virtual void Quit() {}

    //
    // Config
    //
public:
    struct Config
    {
        int width = 960;
        int height = 540;
    };

    void Configurate(std::function<void(Config&)> configurator);
    const Config& GetConfig() const;

private:
    Config config_;

    //
    // Timing
    //
public:
    void Reset();
    void Tick();

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
};

}; // namespace sketch