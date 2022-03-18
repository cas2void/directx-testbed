#include <string>
#include <stdexcept>
#include <iostream>
#include <chrono>

#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <DirectXMath.h>

#include "Launcher.h"
#include "ShadersVS.h"
#include "ShadersPS.h"

using Microsoft::WRL::ComPtr;

inline std::string HrToString(HRESULT hr, const std::string& context)
{
    char str[64] = {};
    sprintf_s(str, "HRESULT of 0x%08X", static_cast<unsigned int>(hr));
    std::string result(str);
    if (context.length() > 0)
    {
        result += ": ";
        result += context;
    }
    return result;
}

// Helper class for COM exceptions
// https://docs.microsoft.com/en-us/windows/win32/seccrypto/common-hresult-values
// https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-error
class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr, const std::string& context) : std::runtime_error(HrToString(hr, context)), result_(hr) {}
    HRESULT Error() const { return result_; }

private:
    const HRESULT result_;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr, const std::string& context = "")
{
    if (FAILED(hr))
    {
        throw HrException(hr, context);
    }
}

class Fullscreen : public sketch::SketchBase
{
    static const UINT kNumSwapChainBuffers = 2;

    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<IDXGISwapChain3> swapChain_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    UINT rtvDescriptorSize_;
    ComPtr<ID3D12Resource> swapChainBuffers_[kNumSwapChainBuffers];
    ComPtr<ID3D12CommandAllocator> commandAllocator_;
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<ID3D12Fence> fence_;
    UINT64 fenceValue_;
    HANDLE fenceEventHandle_;
    ComPtr<ID3D12RootSignature> rootSignature_;
    ComPtr<ID3D12PipelineState> pipelineState_;
    ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;

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
        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&dxgiFactory6)), "CreateDXGIFactory2");

        // Adapter
        ComPtr<IDXGIAdapter> adapter;
        ThrowIfFailed(dxgiFactory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)), "EnumAdapterByGpuPreference");

        // Device
        ComPtr<ID3D12Device> device;
        ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)), "D3D12CreateDevice");

        // MSAA support
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels = {};
        qualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        qualityLevels.SampleCount = 4;
        qualityLevels.NumQualityLevels = 0;
        ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)), "CheckFeatureSupport");

        // Command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)), "CreateCommandQueue");

        // Swap chain
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = GetConfig().width;
        swapChainDesc.Height = GetConfig().height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = kNumSwapChainBuffers;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        // Support Vsync off
        // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays
        BOOL allowTearing = FALSE;
        ThrowIfFailed(dxgiFactory6->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)), "CheckFeatureSupport");
        swapChainDesc.Flags = GetConfig().vsync ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(dxgiFactory6->CreateSwapChainForHwnd(commandQueue_.Get(), launcher::GetMainWindow(), &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf()), "CreateSwapChainForHwnd");
        ThrowIfFailed(swapChain->QueryInterface(IID_PPV_ARGS(&swapChain_)), "QueryInterface");

        // Descriptor heaps
        // 
        // Describe and create a render target view (RTV) descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = kNumSwapChainBuffers;

        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)), "CreateDescriptorHeap");

        // Descriptor size
        rtvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Create frame resources, such as RTV/DSV, for each back buffer.
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->GetCPUDescriptorHandleForHeapStart());
        for (UINT i = 0; i < kNumSwapChainBuffers; i++)
        {
            ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffers_[i])), "GetBuffer");
            device->CreateRenderTargetView(swapChainBuffers_[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvDescriptorSize_);
        }

        // Root Signagure, consisting of emptry root parameter.

        // Allow input layout and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(0, nullptr, 0, nullptr, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), nullptr), "D3D12SerializeVersionedRootSignature");
        ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature_)), "CreateRootSignature");

        // Compile and load shaders
        // Already compiled as g_VSMain, g_PSMain

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{ inputElementDesc, _countof(inputElementDesc) };

        // Pipeline state object
        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = inputLayoutDesc;
        psoDesc.pRootSignature = rootSignature_.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(g_VSMain, sizeof(g_VSMain));
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(g_PSMain, sizeof(g_PSMain));
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_)), "CreateGraphicsPipelineState");

        // Command allocator
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_)), "CreateCommandAllocator");

        // Command list
        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), pipelineState_.Get(), IID_PPV_ARGS(&commandList_)), "CreateCommandList");

        // Create the vertex buffer.

        float aspectRatio = (float)GetConfig().width / GetConfig().height;
        // Define the geometry for a triangle.
        Vertex quadVertices[] =
        {
            { { -0.25f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, 0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(quadVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not recommended.
        // Every time the GPU needs it, the upload heap will be marshalled over. Use default heaps instead.
        CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

        CD3DX12_HEAP_PROPERTIES defaultProperty(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->CreateCommittedResource(&defaultProperty, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertexBuffer_)), "CreateCommittedResource");

        ComPtr<ID3D12Resource> vertexBufferUpload;
        CD3DX12_HEAP_PROPERTIES uploadPropety(D3D12_HEAP_TYPE_UPLOAD);
        ThrowIfFailed(device->CreateCommittedResource(&uploadPropety, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBufferUpload)), "CreateCommittedResource");

        // Copy the triangle data to the vertex buffer in upload heap.
        UINT8* vertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(vertexBufferUpload->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)), "Map");
        memcpy(vertexDataBegin, quadVertices, sizeof(quadVertices));
        vertexBufferUpload->Unmap(0, nullptr);

        // 将顶点数据由 upload heap 拷贝至 default heap
        // 这里只是记录指令，实际执行在本函数的末尾。因此，必须保证 upload heap 中分配的资源在指令执行时是有效的。
        commandList_->CopyBufferRegion(vertexBuffer_.Get(), 0, vertexBufferUpload.Get(), 0, vertexBufferSize);

        // 切换顶点缓冲的状态
        CD3DX12_RESOURCE_BARRIER toVertexBufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer_.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        commandList_->ResourceBarrier(1, &toVertexBufferBarrier);

        // Initialize the vertex buffer view.
        vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
        vertexBufferView_.StrideInBytes = sizeof(Vertex);
        vertexBufferView_.SizeInBytes = vertexBufferSize;

        // Command lists are created in the recording state.
        // Close the resource creation command list and execute it to begin the vertex buffer copy into the default heap.
        ThrowIfFailed(commandList_->Close(), "Close command list");
        ID3D12CommandList* commandLists[] = { commandList_.Get() };
        commandQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);

        // Fence
        UINT64 initialFenceValue = 0;
        ThrowIfFailed(device->CreateFence(initialFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)), "CreateFence");
        fenceValue_ = initialFenceValue + 1;
        fenceEventHandle_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);

        ShowInfo();
        FlushCommandQueue();
    }

    virtual void Update() override
    {
        ShowInfo();
        // Command list allocators can only be reset when the associated command lists have finished execution on the GPU.
        // Apps shoud use fences to determin GPU execution progress, which we will do at the end of this function.
        ThrowIfFailed(commandAllocator_->Reset(), "Reset command allocator");

        // After ExecuteCommandList() has been called on a particular command list,
        // that command list can then be reset at any time before re-recoding.
        ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), pipelineState_.Get()), "Reset command list");

        // Indicate the the back buffer will be used as a render target.
        const UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
        CD3DX12_RESOURCE_BARRIER toRenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffers_[backBufferIndex].Get(), 
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList_->ResourceBarrier(1, &toRenderBarrier);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->GetCPUDescriptorHandleForHeapStart(), backBufferIndex, rtvDescriptorSize_);

        commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Set necessary state.
        commandList_->SetGraphicsRootSignature(rootSignature_.Get());

        CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(GetConfig().width), static_cast<float>(GetConfig().height));
        CD3DX12_RECT scissorRect(0, 0, GetConfig().width, GetConfig().height);
        commandList_->RSSetViewports(1, &viewport);
        commandList_->RSSetScissorRects(1, &scissorRect);

        // Record commands
        const FLOAT clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
        commandList_->DrawInstanced(4, 1, 0, 0);

        // Indicate that the back buffer will now be used to present.
        CD3DX12_RESOURCE_BARRIER toPresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffers_[backBufferIndex].Get(), 
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList_->ResourceBarrier(1, &toPresentBarrier);

        // Command list is expected to be closed before calling Reset again.
        ThrowIfFailed(commandList_->Close(), "Clost command list");

        // Execulte the command list
        ID3D12CommandList* commandLists[] = { commandList_.Get() };
        commandQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);

        // Swap buffers
        if (GetConfig().vsync)
        {
            ThrowIfFailed(swapChain_->Present(1, 0), "Present");
        }
        else
        {
            ThrowIfFailed(swapChain_->Present(0, DXGI_PRESENT_ALLOW_TEARING), "Present");
        }

        FlushCommandQueue();
    }

    virtual void Quit() override
    {
        FlushCommandQueue();
        CloseHandle(fenceEventHandle_);
    }

    void ShowInfo()
    {
        
    }

    void FlushCommandQueue()
    {
        // Add an instruction to the command queue to set a new fence point by instructing 'fence_' to wait for the 'fenceValue_'.
        // `fence_` value won't be set by GPU until it finishes processing all the commands prior to this `Signal()`.
        const UINT64 fenceValueToWaitFor = fenceValue_;
        ThrowIfFailed(commandQueue_->Signal(fence_.Get(), fenceValueToWaitFor), "Signal");
        fenceValue_++;

        // Wait until the GPU has completed commands up to this fence point.
        if (fence_->GetCompletedValue() < fenceValueToWaitFor)
        {
            // Fire event when GPU hits current fence.
            ThrowIfFailed(fence_->SetEventOnCompletion(fenceValueToWaitFor, fenceEventHandle_), "SetEventOnCompletion");
            // Wait until the created event is fired
            WaitForSingleObject(fenceEventHandle_, INFINITE);
        }
    }
};

CREATE_SKETCH(Fullscreen,
    [](sketch::SketchBase::Config& config)
    {
        config.width = 800;
        config.height = 450;
        config.vsync = false;
    }
)