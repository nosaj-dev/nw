# League of Legends Range Overlay

Overlay that renders auto-attack and ability ranges in League of Legends by scanning process memory and hooking DirectX 11. **High ban risk** – intended for educational research only.

## Roadmap

1. Reverse engineer memory layout to locate local player and ability structures (signatures, pointer chains, offsets)
2. Build pattern-based memory scanner
3. Hook `IDXGISwapChain::Present` via MinHook and render overlay primitives (D3D11)
4. Implement anti-detection tactics (VMT hook, thread hijack, randomized polling)
5. ImGui configuration (toggles, colors, hotkeys)
6. Profiling & stealth validation

## Build

```ps1
cmake -S . -B build -G "Ninja"
cmake --build build --config Release
```

Dependencies fetched via CMake (MinHook, ImGui planned). Ensure MSVC toolchain and Windows SDK are installed.
