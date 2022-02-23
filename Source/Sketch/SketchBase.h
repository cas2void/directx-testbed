#pragma once

#include <functional>

namespace sketch
{

class SketchBase
{
public:
    struct Config
    {
        int width = 960;
        int height = 540;
    };

    virtual void Init() {}
    virtual void Update() {}
    virtual void Quit() {}

    void Configurate(std::function<void(Config&)> configurator);
    const Config& GetConfig() const;

protected:
    Config config_;
};

}; // namespace sketch