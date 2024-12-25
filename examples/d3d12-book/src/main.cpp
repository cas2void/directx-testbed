// ReSharper disable CppClangTidyHicppMultiwayPathsCovered
#include "sketch-base.h"
#include "launcher.h"

#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "d3dx12_root_signature.h"
#include "d3dx12_barriers.h"

using Microsoft::WRL::ComPtr;

#pragma region ErrorHandling
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <cassert>

namespace
{
std::string ComResultToString(const HRESULT result)
{
    std::stringstream stream;
    stream << "HRESULT: 0x" << std::setfill('0') << std::setw(8) << std::hex << static_cast<unsigned int>(result);

    return stream.str();
}
}
#pragma endregion ErrorHandling

class D3D12BookSketch final : public engine::SketchBase
{
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> command_queue_;
    ComPtr<IDXGISwapChain3> swap_chain_;
    ComPtr<ID3D12DescriptorHeap> rtv_heap_;
    ComPtr<ID3D12CommandAllocator> command_allocator_;
    ComPtr<ID3D12GraphicsCommandList> command_list_;
    ComPtr<ID3D12Fence> fence_;
    UINT64 current_fence_value_ = 0;

    void OnInit() override
    {
        UINT dxgi_factory_flag = 0;

#ifndef NDEBUG
        ComPtr<ID3D12Debug> debug_controller;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
        {
            debug_controller->EnableDebugLayer();
        }

        // Enable additional debug layers
        dxgi_factory_flag |= DXGI_CREATE_FACTORY_DEBUG;
#endif

        ComPtr<IDXGIFactory6> dxgi_factory6;
        Verify(CreateDXGIFactory2(dxgi_factory_flag, IID_PPV_ARGS(&dxgi_factory6)));

        ComPtr<IDXGIAdapter> adapter;
        Verify(dxgi_factory6->EnumAdapterByGpuPreference(0,
                                                         DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                         IID_PPV_ARGS(&adapter)));

        Verify(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)));

        constexpr D3D12_COMMAND_QUEUE_DESC queue_desc = {};
        Verify(device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_)));

        constexpr int swap_chain_buffer_count = 2;

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
        swap_chain_desc.Width = 0;
        swap_chain_desc.Height = 0;
        swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.SampleDesc.Count = 1;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount = swap_chain_buffer_count;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        auto allow_tearing = 0;
        Verify(dxgi_factory6->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                  &allow_tearing,
                                                  sizeof(allow_tearing)));
        swap_chain_desc.Flags = (!GetConfig().vsync && allow_tearing > 0) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> swap_chain1;
        Verify(dxgi_factory6->CreateSwapChainForHwnd(command_queue_.Get(),
                                                     engine::GetMainWindow(),
                                                     &swap_chain_desc,
                                                     nullptr,
                                                     nullptr,
                                                     swap_chain1.GetAddressOf()));
        Verify(swap_chain1->QueryInterface(IID_PPV_ARGS(&swap_chain_)));

        D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
        rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_heap_desc.NumDescriptors = swap_chain_buffer_count;

        Verify(device_->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&rtv_heap_)));

        const auto rtv_descriptor_size = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap_->GetCPUDescriptorHandleForHeapStart());
        for (auto i = 0; i < swap_chain_buffer_count; i++)
        {
            ComPtr<ID3D12Resource> swap_chain_buffer;
            Verify(swap_chain_->GetBuffer(i, IID_PPV_ARGS(&swap_chain_buffer)));
            device_->CreateRenderTargetView(swap_chain_buffer.Get(), nullptr, rtv_handle);
            rtv_handle.Offset(1, rtv_descriptor_size);
        }

        Verify(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_)));
        Verify(device_->CreateCommandList(0,
                                          D3D12_COMMAND_LIST_TYPE_DIRECT,
                                          command_allocator_.Get(),
                                          nullptr,
                                          IID_PPV_ARGS(&command_list_)));
        Verify(command_list_->Close());

        constexpr UINT64 initial_fence_value = 0;
        Verify(device_->CreateFence(initial_fence_value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
        current_fence_value_ = initial_fence_value + 1;

        WaitForGpu();
    }

    void OnTick() override
    {
        Verify(command_allocator_->Reset());
        Verify(command_list_->Reset(command_allocator_.Get(), nullptr));

        const auto back_buffer_index = swap_chain_->GetCurrentBackBufferIndex();

        ComPtr<ID3D12Resource> swap_chain_back_buffer;
        Verify(swap_chain_->GetBuffer(back_buffer_index, IID_PPV_ARGS(&swap_chain_back_buffer)));

        const auto to_render_target_barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            swap_chain_back_buffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        command_list_->ResourceBarrier(1, &to_render_target_barrier);

        const auto rtv_descriptor_size = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_heap_->GetCPUDescriptorHandleForHeapStart(),
                                                       static_cast<INT>(back_buffer_index),
                                                       rtv_descriptor_size);

        constexpr FLOAT clear_color[] = {0.0f, 0.2f, 0.4f, 1.0f};
        command_list_->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);

        const auto to_present_barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            swap_chain_back_buffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
        command_list_->ResourceBarrier(1, &to_present_barrier);

        Verify(command_list_->Close());

        ID3D12CommandList* command_lists[] = {command_list_.Get()};
        command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists);

        Verify(swap_chain_->Present(1, 0));

        WaitForGpu();
    }

    void OnQuit() override
    {
        ComPtr<ID3D12DebugDevice> debug_device;
        Verify(device_->QueryInterface(IID_PPV_ARGS(&debug_device)));
        Verify(debug_device->ReportLiveDeviceObjects(D3D12_RLDO_IGNORE_INTERNAL));
    }

    void WaitForGpu()
    {
        const auto fence_value_to_wait_for = current_fence_value_;
        Verify(command_queue_->Signal(fence_.Get(), fence_value_to_wait_for));
        current_fence_value_++;

        if (fence_->GetCompletedValue() < fence_value_to_wait_for)
        {
            const auto fence_event = CreateEvent(nullptr,
                                                 FALSE,
                                                 FALSE,
                                                 nullptr);
            if (!fence_event)
            {
                Verify(HRESULT_FROM_WIN32(GetLastError()));
            }

            Verify(fence_->SetEventOnCompletion(fence_value_to_wait_for, fence_event));
            WaitForSingleObject(fence_event, INFINITE);
            CloseHandle(fence_event);
        }
    }

    void Verify(const HRESULT result) const
    {
        if (FAILED(result))
        {
            std::string error_string;
            switch (result)
            {
            case E_FAIL:
                error_string = "E_FAIL(Unspecified failure)";
                break;
            case DXGI_ERROR_DEVICE_REMOVED:
                error_string = "DXGI_ERROR_DEVICE_REMOVED";
                switch (device_->GetDeviceRemovedReason())
                {
                case DXGI_ERROR_INVALID_CALL:
                    error_string += " | DXGI_ERROR_INVALID_CALL";
                    break;
                default:
                    error_string += " | " + ComResultToString(result);
                }
                break;
            default:
                error_string = ComResultToString(result);
            }

            assert(false);
            throw std::runtime_error(error_string);
        }
    }
};

LAUNCH_SKETCH(D3D12BookSketch)
