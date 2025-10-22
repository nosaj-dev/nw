#include "dx_hook.hpp"
#include "memory_scanner.hpp"
#include <Windows.h>


namespace {

HMODULE g_module_handle = nullptr;

DWORD WINAPI OverlayBootstrap(LPVOID) {
  DisableThreadLibraryCalls(g_module_handle);

  MemoryScanner scanner;
  if (!scanner.Attach("League of Legends.exe")) {
    return 0;
  }

  DXHook overlay(scanner);
  if (!overlay.Initialize()) {
    return 0;
  }

  while (!overlay.ShouldShutdown()) {
    overlay.RenderFrame();
    Sleep(10); // ~100 FPS update budget
  }

  overlay.Shutdown();
  scanner.Detach();
  return 0;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    g_module_handle = hModule;
    HANDLE thread =
        CreateThread(nullptr, 0, OverlayBootstrap, nullptr, 0, nullptr);
    if (thread) {
      CloseHandle(thread);
    }
  }

  if (ul_reason_for_call == DLL_PROCESS_DETACH) {
    // TODO: Implement explicit shutdown path when needed.
  }

  return TRUE;
}
