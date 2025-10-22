#include "dx_hook.hpp"

#include "memory_scanner.hpp"

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <array>
#include <atomic>
#include <cmath>

#include <minhook.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace {

using PresentFn = HRESULT(__stdcall *)(IDXGISwapChain *, UINT, UINT);

PresentFn g_original_present = nullptr;
DXHook *g_overlay_instance = nullptr;

constexpr float PI = 3.14159265358979323846f;

bool CreateRenderTarget(IDXGISwapChain *swap_chain,
                        ID3D11RenderTargetView **out_rtv) {
  ID3D11Texture2D *back_buffer = nullptr;
  if (FAILED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
    return false;
  }

  ID3D11Device *device = nullptr;
  swap_chain->GetDevice(__uuidof(ID3D11Device),
                        reinterpret_cast<void **>(&device));
  if (!device) {
    back_buffer->Release();
    return false;
  }

  const auto hr = device->CreateRenderTargetView(back_buffer, nullptr, out_rtv);
  back_buffer->Release();
  device->Release();
  return SUCCEEDED(hr);
}

void DrawCircle(ID3D11DeviceContext *context, ID3D11RenderTargetView *rtv,
                float center_x, float center_y, float radius,
                const std::array<float, 4> &color) {
  // TODO: Replace with proper primitive rendering. Placeholder for now.
  static_cast<void>(context);
  static_cast<void>(rtv);
  static_cast<void>(center_x);
  static_cast<void>(center_y);
  static_cast<void>(radius);
  static_cast<void>(color);
}

HRESULT __stdcall PresentHook(IDXGISwapChain *swap_chain, UINT sync_interval,
                              UINT flags) {
  if (!g_overlay_instance) {
    return g_original_present(swap_chain, sync_interval, flags);
  }

  g_overlay_instance->RenderFrame();
  return g_original_present(swap_chain, sync_interval, flags);
}

} // namespace

struct DXHook::Impl {
  IDXGISwapChain *swap_chain = nullptr;
  ID3D11Device *device = nullptr;
  ID3D11DeviceContext *context = nullptr;
  ID3D11RenderTargetView *render_target_view = nullptr;
  std::atomic<bool> ready = false;
};

DXHook::DXHook(MemoryScanner &scanner)
    : impl_(std::make_unique<Impl>()), scanner_(scanner) {}

DXHook::~DXHook() { Shutdown(); }

bool DXHook::Initialize() {
  if (!InstallHook()) {
    return false;
  }

  g_overlay_instance = this;
  return true;
}

bool DXHook::InstallHook() {
  // Attempt to create a dummy device to identify swap chain vtable entries.
  DXGI_SWAP_CHAIN_DESC swap_desc{};
  swap_desc.BufferCount = 1;
  swap_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_desc.OutputWindow = GetDesktopWindow();
  swap_desc.SampleDesc.Count = 1;
  swap_desc.Windowed = TRUE;

  ID3D11Device *device = nullptr;
  ID3D11DeviceContext *context = nullptr;
  IDXGISwapChain *swap_chain = nullptr;

  const auto hr = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
      D3D11_SDK_VERSION, &swap_desc, &swap_chain, &device, nullptr, &context);

  if (FAILED(hr)) {
    return false;
  }

  auto **vtable = *reinterpret_cast<void ***>(swap_chain);
  auto present = reinterpret_cast<PresentFn>(vtable[8]);

  device->Release();
  context->Release();
  swap_chain->Release();

  if (MH_Initialize() != MH_OK) {
    return false;
  }

  if (MH_CreateHook(reinterpret_cast<void *>(present), &PresentHook,
                    reinterpret_cast<void **>(&g_original_present)) != MH_OK) {
    MH_Uninitialize();
    return false;
  }

  if (MH_EnableHook(reinterpret_cast<void *>(present)) != MH_OK) {
    MH_RemoveHook(reinterpret_cast<void *>(present));
    MH_Uninitialize();
    return false;
  }

  return true;
}

void DXHook::RenderFrame() {
  if (auto snapshot = scanner_.SnapshotLocalChampion()) {
    DrawSnapshot(*snapshot);
  }
}

void DXHook::DrawSnapshot(const ChampionSnapshot &snapshot) {
  // TODO: Project world coordinates to screen space and issue draw calls.
  static_cast<void>(snapshot);
}

void DXHook::Shutdown() {
  RemoveHook();
  g_overlay_instance = nullptr;
}

void DXHook::RemoveHook() {
  MH_DisableHook(MH_ALL_HOOKS);
  MH_RemoveHook(MH_ALL_HOOKS);
  MH_Uninitialize();
}
