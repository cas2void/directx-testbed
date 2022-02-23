#include <string>
#include <stdexcept>

#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>

#include "Launcher.h"

using Microsoft::WRL::ComPtr;

inline std::string HrToString(HRESULT hr)
{
    char str[64] = {};
    sprintf_s(str, "HRESULT of 0x%08X", static_cast<unsigned int>(hr));
    return std::string(str);
}

// Helper class for COM exceptions
// https://docs.microsoft.com/en-us/windows/win32/seccrypto/common-hresult-values
// https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-error
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

struct CD3DX12_DEFAULT {};

//------------------------------------------------------------------------------------------------
struct CD3DX12_CPU_DESCRIPTOR_HANDLE : public D3D12_CPU_DESCRIPTOR_HANDLE
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE() = default;
    explicit CD3DX12_CPU_DESCRIPTOR_HANDLE(const D3D12_CPU_DESCRIPTOR_HANDLE& o) noexcept :
        D3D12_CPU_DESCRIPTOR_HANDLE(o)
    {}
    CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT) noexcept { ptr = 0; }
    CD3DX12_CPU_DESCRIPTOR_HANDLE(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other, INT offsetScaledByIncrementSize) noexcept
    {
        InitOffsetted(other, offsetScaledByIncrementSize);
    }
    CD3DX12_CPU_DESCRIPTOR_HANDLE(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other, INT offsetInDescriptors, UINT descriptorIncrementSize) noexcept
    {
        InitOffsetted(other, offsetInDescriptors, descriptorIncrementSize);
    }
    CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetInDescriptors, UINT descriptorIncrementSize) noexcept
    {
        ptr = SIZE_T(INT64(ptr) + INT64(offsetInDescriptors) * INT64(descriptorIncrementSize));
        return *this;
    }
    CD3DX12_CPU_DESCRIPTOR_HANDLE& Offset(INT offsetScaledByIncrementSize) noexcept
    {
        ptr = SIZE_T(INT64(ptr) + INT64(offsetScaledByIncrementSize));
        return *this;
    }
    bool operator==(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other) const noexcept
    {
        return (ptr == other.ptr);
    }
    bool operator!=(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& other) const noexcept
    {
        return (ptr != other.ptr);
    }
    CD3DX12_CPU_DESCRIPTOR_HANDLE& operator=(const D3D12_CPU_DESCRIPTOR_HANDLE& other) noexcept
    {
        ptr = other.ptr;
        return *this;
    }

    inline void InitOffsetted(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize) noexcept
    {
        InitOffsetted(*this, base, offsetScaledByIncrementSize);
    }

    inline void InitOffsetted(_In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize) noexcept
    {
        InitOffsetted(*this, base, offsetInDescriptors, descriptorIncrementSize);
    }

    static inline void InitOffsetted(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& handle, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetScaledByIncrementSize) noexcept
    {
        handle.ptr = SIZE_T(INT64(base.ptr) + INT64(offsetScaledByIncrementSize));
    }

    static inline void InitOffsetted(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& handle, _In_ const D3D12_CPU_DESCRIPTOR_HANDLE& base, INT offsetInDescriptors, UINT descriptorIncrementSize) noexcept
    {
        handle.ptr = SIZE_T(INT64(base.ptr) + INT64(offsetInDescriptors) * INT64(descriptorIncrementSize));
    }
};

//------------------------------------------------------------------------------------------------
struct CD3DX12_RESOURCE_BARRIER : public D3D12_RESOURCE_BARRIER
{
    CD3DX12_RESOURCE_BARRIER() = default;
    explicit CD3DX12_RESOURCE_BARRIER(const D3D12_RESOURCE_BARRIER& o) noexcept :
        D3D12_RESOURCE_BARRIER(o)
    {}
    static inline CD3DX12_RESOURCE_BARRIER Transition(
        _In_ ID3D12Resource* pResource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_BARRIER_FLAGS flags = D3D12_RESOURCE_BARRIER_FLAG_NONE) noexcept
    {
        CD3DX12_RESOURCE_BARRIER result = {};
        D3D12_RESOURCE_BARRIER& barrier = result;
        result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        result.Flags = flags;
        barrier.Transition.pResource = pResource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = subresource;
        return result;
    }
    static inline CD3DX12_RESOURCE_BARRIER Aliasing(
        _In_ ID3D12Resource* pResourceBefore,
        _In_ ID3D12Resource* pResourceAfter) noexcept
    {
        CD3DX12_RESOURCE_BARRIER result = {};
        D3D12_RESOURCE_BARRIER& barrier = result;
        result.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
        barrier.Aliasing.pResourceBefore = pResourceBefore;
        barrier.Aliasing.pResourceAfter = pResourceAfter;
        return result;
    }
    static inline CD3DX12_RESOURCE_BARRIER UAV(
        _In_ ID3D12Resource* pResource) noexcept
    {
        CD3DX12_RESOURCE_BARRIER result = {};
        D3D12_RESOURCE_BARRIER& barrier = result;
        result.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.UAV.pResource = pResource;
        return result;
    }
};

class Ch04_Initialization : public sketch::SketchBase
{
    static const UINT kSwapChainBufferCount = 2;

    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<ID3D12CommandAllocator> commandAllocator_;
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<ID3D12Fence> fence_;
    UINT64 fenceValue_;
    ComPtr<ID3D12Resource> swapChainBuffers_[kSwapChainBufferCount];
    ComPtr<IDXGISwapChain3> swapChain_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    UINT rtvDescriptorSize_;

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

        // MSAA support
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels = {};
        qualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        qualityLevels.SampleCount = 4;
        //qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
        qualityLevels.NumQualityLevels = 0;
        ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)));

        // Command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        //queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)));

        // Command allocator
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_)));

        // Command list
        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_)));

        // Command lists are created in the recording state, but there is nothing to record yet.
        // The first time we refer to the command list we will Reset it, and it is expected to be closed before calling Reset.
        ThrowIfFailed(commandList_->Close());

        // Swap chain
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = config_.width;
        swapChainDesc.Height = config_.height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = kSwapChainBufferCount;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(dxgiFactory6->CreateSwapChainForHwnd(commandQueue_.Get(), launcher::GetMainWindow(), &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf()));
        ThrowIfFailed(swapChain->QueryInterface(IID_PPV_ARGS(&swapChain_)));

        // Descriptor heaps
        // 
        // Describe and create a render target view (RTV) descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = kSwapChainBufferCount;
        //rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)));

        // Descriptor size
        rtvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Create frame resources, such as RTV/DSV, for each frame buffer.
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->GetCPUDescriptorHandleForHeapStart());
        for (UINT i = 0; i < kSwapChainBufferCount; i++)
        {
            ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffers_[i])));
            device->CreateRenderTargetView(swapChainBuffers_[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvDescriptorSize_);
        }

        // Fence
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
        fenceValue_ = 1;
    }

    virtual void Update() override
    {
        // Command list allocators can only be reset when the associated command lists have finished execution on the GPU.
        // Apps shoud use fences to determin GPU execution progress, which we will do at the end of this function.
        ThrowIfFailed(commandAllocator_->Reset());

        // After ExecuteCommandList() has been called on a particular command list,
        // that command list can then be reset at any time before re-recoding.
        ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));

        const UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

        // Indicate the the back buffer will be used as a render target.
        CD3DX12_RESOURCE_BARRIER toRenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffers_[backBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList_->ResourceBarrier(1, &toRenderBarrier);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->GetCPUDescriptorHandleForHeapStart(), backBufferIndex, rtvDescriptorSize_);

        // Record commands
        const FLOAT clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f};
        commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        // Indicate that the back buffer will now be used to present.
        CD3DX12_RESOURCE_BARRIER toPresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffers_[backBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList_->ResourceBarrier(1, &toPresentBarrier);

        // Command list is expected to be closed before calling Reset again.
        ThrowIfFailed(commandList_->Close());

        // Execulte the command list
        ID3D12CommandList* commandLists[] = { commandList_.Get() };
        commandQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);

        // Swap buffers
        ThrowIfFailed(swapChain_->Present(1, 0));

        // Add an instruction to the command queue to set a new fence point by
        // instructing 'fence_' to wait for the 'fenceValue_'.
        const UINT64 fenceValueToWaitFor = fenceValue_;
        ThrowIfFailed(commandQueue_->Signal(fence_.Get(), fenceValueToWaitFor));
        fenceValue_++;
        
        // Wait until the GPU has completed commands up to this fence point.
        if (fence_->GetCompletedValue() < fenceValueToWaitFor)
        {
            HANDLE eventHandle = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            // Fire event when GPU hits current fence.
            ThrowIfFailed(fence_->SetEventOnCompletion(fenceValueToWaitFor, eventHandle));

            // Wait until the created event is fired
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
    }
};

CREATE_SKETCH(Ch04_Initialization,
    [](sketch::SketchBase::Config& config)
    {
        config.width = 800;
        config.height = 450;
    }
)