#include <string>
#include <stdexcept>
#include <iostream>

#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>

#include "SketchBase.h"
#include "Launcher.h"

using Microsoft::WRL::ComPtr;

inline std::string HrToString(HRESULT hr)
{
    char str[64] = {};
    sprintf_s(str, "HRESULT of 0x%08X", static_cast<unsigned int>(hr));
    return std::string(str);
}

// Helper class for COM exceptions
class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), result_(hr) {}
    HRESULT Error() const { return result_; }

private:
    const HRESULT result_;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

class Ch04_Initialization : public sketch::SketchBase
{
public:
    virtual void Init() override
    {
        UINT dxgiFactoryFlag = 0;

#ifndef NDEBUG
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        // When the debug layer is enabled, D3D will send debug messages to the Visual Studio output window like the following :
        // D3D12 ERROR : ID3D12CommandList::Reset : Reset fails because the command list was not closed.
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

                // Enable additional debug layers
                dxgiFactoryFlag |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif // !NDEBUG

        // Factory
        ComPtr<IDXGIFactory6> dxgiFactory6;
        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&dxgiFactory6)));

        // Adapter
        ComPtr<IDXGIAdapter> adapter;
        ThrowIfFailed(dxgiFactory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));

        // Device
        ComPtr<ID3D12Device> device;
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

        // Fence
        ComPtr<ID3D12Fence> fence;
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

        // Descriptor size
        UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // MSAA support
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
        qualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        qualityLevels.SampleCount = 0;
        qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        qualityLevels.NumQualityLevels = 0;
        ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)));

    }
};

CREATE_SKETCH(Ch04_Initialization,
    [](sketch::SketchConfig& config)
    {
        config.width = 1280;
        config.height = 720;
    }
)