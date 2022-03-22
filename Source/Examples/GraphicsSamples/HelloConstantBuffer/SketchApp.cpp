#include <string>
#include <stdexcept>
#include <iostream>

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

inline void ThrowIfFailed(bool condition)
{
    if (!condition)
    {
        throw HrException(E_FAIL);
    }
}

class HelloConstantBuffer : public sketch::SketchBase
{
    static const UINT kSwapChainBufferCount = 2;

    struct Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    struct SceneConstantBuffer
    {
        DirectX::XMFLOAT4 offset;
        float padding[60]; // Padding so the constant buffer is 256-byte aligned.
    };
    static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<IDXGISwapChain3> swapChain_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    ComPtr<ID3D12DescriptorHeap> cbvHeap_;
    UINT rtvDescriptorSize_;
    ComPtr<ID3D12Resource> swapChainBuffers_[kSwapChainBufferCount];
    ComPtr<ID3D12CommandAllocator> commandAllocator_;
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<ID3D12Fence> fence_;
    UINT64 fenceValue_;
    ComPtr<ID3D12RootSignature> rootSignature_;
    ComPtr<ID3D12PipelineState> pipelineState_;
    ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;
    SceneConstantBuffer constantBufferData_;
    ComPtr<ID3D12Resource> constantBuffer_;
    UINT8* cbvDataBegin_;

public:
    virtual void OnInit() override
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
        qualityLevels.NumQualityLevels = 0;
        ThrowIfFailed(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(qualityLevels)));

        // Command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue_)));

        // Swap chain
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = GetConfig().Width;
        swapChainDesc.Height = GetConfig().Height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = kSwapChainBufferCount;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        // Support Vsync off
        // https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays
        BOOL allowTearing = FALSE;
        ThrowIfFailed(dxgiFactory6->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)));
        swapChainDesc.Flags = GetConfig().Vsync ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(dxgiFactory6->CreateSwapChainForHwnd(commandQueue_.Get(), launcher::GetMainWindow(), &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf()));
        ThrowIfFailed(swapChain->QueryInterface(IID_PPV_ARGS(&swapChain_)));

        // Disable Alt+Enter fullscreen transitions offered by IDXGIFactory
        //
        // https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory-makewindowassociation?redirectedfrom=MSDN#remarks
        // Applications that want to handle mode changes or Alt+Enter themselves should call 
        // MakeWindowAssociation with the DXGI_MWA_NO_WINDOW_CHANGES flag **AFTER** swap chain creation.
        // Ensures that DXGI will not interfere with application's handling of window mode changes or Alt+Enter.
        ThrowIfFailed(dxgiFactory6->MakeWindowAssociation(launcher::GetMainWindow(), DXGI_MWA_NO_ALT_ENTER));

        // Descriptor heaps
        // 
        // Describe and create a render target view (RTV) descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = kSwapChainBufferCount;

        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_)));

        // Descriptor size
        rtvDescriptorSize_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Describe and create a constant buffer view (CBV) descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
        cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvHeapDesc.NumDescriptors = 1;
        cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&cbvHeap_)));

        // Create frame resources, such as RTV/DSV, for each back buffer.
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->GetCPUDescriptorHandleForHeapStart());
        for (UINT i = 0; i < kSwapChainBufferCount; i++)
        {
            ThrowIfFailed(swapChain_->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffers_[i])));
            device->CreateRenderTargetView(swapChainBuffers_[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvDescriptorSize_);
        }

        // Root Signagure, consisting of a descriptor table with a single CBV
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_VERTEX);

        // Allow input layout and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = 
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
        
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), nullptr));
        ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature_)));

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

        ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_)));

        // Command allocator
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_)));

        // Command list
        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), pipelineState_.Get(), IID_PPV_ARGS(&commandList_)));

        // Command lists are created in the recording state, but there is nothing to record yet.
        // The first time we refer to the command list we will Reset it, and it is expected to be closed before calling Reset.
        ThrowIfFailed(commandList_->Close());

        // Fence
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
        fenceValue_ = 1;

        // Create the vertex buffer.

        float aspectRatio = (float)GetConfig().Width / GetConfig().Height;
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        CD3DX12_HEAP_PROPERTIES uploadPropety(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        ThrowIfFailed(device->CreateCommittedResource(&uploadPropety, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer_)));

        // Initialize the vertex buffer view.
        // Buffer View的创建可以放在数据拷贝之后或之前，因为所需的信息均为已知。
        vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
        vertexBufferView_.StrideInBytes = sizeof(Vertex);
        vertexBufferView_.SizeInBytes = vertexBufferSize;

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(vertexBuffer_->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        vertexBuffer_->Unmap(0, nullptr);

        // Create the constant buffer

        const UINT constantBufferSize = sizeof(SceneConstantBuffer);

        CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
        ThrowIfFailed(device->CreateCommittedResource(&uploadPropety, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constantBuffer_)));

        // Describe and create a constant buffer view (CBV)
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = constantBuffer_->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = constantBufferSize;

        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(cbvHeap_->GetCPUDescriptorHandleForHeapStart());
        device->CreateConstantBufferView(&cbvDesc, cbvHandle);

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        ThrowIfFailed(constantBuffer_->Map(0, &readRange, reinterpret_cast<void**>(&cbvDataBegin_)));
        memcpy(cbvDataBegin_, &constantBufferData_, constantBufferSize);

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

    virtual void OnUpdate() override
    {
        const float translationSpeed = 0.3f;
        const float offsetBounds = 1.25f;

        constantBufferData_.offset.x += translationSpeed * GetDeltaTime();
        if (constantBufferData_.offset.x > offsetBounds)
        {
            constantBufferData_.offset.x = -offsetBounds;
        }
        memcpy(cbvDataBegin_, &constantBufferData_, sizeof(constantBufferData_));

        // Command list allocators can only be reset when the associated command lists have finished execution on the GPU.
        // Apps shoud use fences to determin GPU execution progress, which we will do at the end of this function.
        ThrowIfFailed(commandAllocator_->Reset());

        // After ExecuteCommandList() has been called on a particular command list,
        // that command list can then be reset at any time before re-recoding.
        ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), pipelineState_.Get()));

        // Indicate the the back buffer will be used as a render target.
        const UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
        CD3DX12_RESOURCE_BARRIER toRenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffers_[backBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList_->ResourceBarrier(1, &toRenderBarrier);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap_->GetCPUDescriptorHandleForHeapStart(), backBufferIndex, rtvDescriptorSize_);

        commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Set necessary state.
        commandList_->SetGraphicsRootSignature(rootSignature_.Get());
        // 绑定数据
        ID3D12DescriptorHeap* descriptorHeaps[] = { cbvHeap_.Get() };
        commandList_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
        commandList_->SetGraphicsRootDescriptorTable(0, cbvHeap_->GetGPUDescriptorHandleForHeapStart());

        CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(GetConfig().Width), static_cast<float>(GetConfig().Height));
        CD3DX12_RECT scissorRect(0, 0, GetConfig().Width, GetConfig().Height);
        commandList_->RSSetViewports(1, &viewport);
        commandList_->RSSetScissorRects(1, &scissorRect);

        // Record commands
        const FLOAT clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f};
        commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList_->IASetVertexBuffers(0, 1, &vertexBufferView_);
        commandList_->DrawInstanced(3, 1, 0, 0);

        // Indicate that the back buffer will now be used to present.
        CD3DX12_RESOURCE_BARRIER toPresentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffers_[backBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList_->ResourceBarrier(1, &toPresentBarrier);

        // Command list is expected to be closed before calling Reset again.
        ThrowIfFailed(commandList_->Close());

        // Execulte the command list
        ID3D12CommandList* commandLists[] = { commandList_.Get() };
        commandQueue_->ExecuteCommandLists(_countof(commandLists), commandLists);

        // Swap buffers
        if (GetConfig().Vsync)
        {
            ThrowIfFailed(swapChain_->Present(1, 0));
        }
        else
        {
            ThrowIfFailed(swapChain_->Present(0, DXGI_PRESENT_ALLOW_TEARING));
        }

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

    virtual void OnQuit() override
    {
        constantBuffer_->Unmap(0, nullptr);
    }
};

CREATE_SKETCH(HelloConstantBuffer,
    [](sketch::SketchBase::Config& config)
    {
        config.Width = 800;
        config.Height = 450;
    }
)