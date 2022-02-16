#include <functional>

namespace sketch
{

struct SketchConfig
{
    int width = 960;
    int height = 540;
};

class SketchBase
{
public:
    virtual void Init();
    virtual void Update(float dt);
    virtual void Quit();

    void Configurate(std::function<void(SketchConfig&)> configurator);
    SketchConfig GetConfig() const;

protected:
    SketchConfig config_;
};

}; // namespace sketch