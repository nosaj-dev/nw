#pragma once

#include <memory>

struct ChampionSnapshot;
class MemoryScanner;

class DXHook {
public:
  explicit DXHook(MemoryScanner &scanner);
  ~DXHook();

  bool Initialize();
  void RenderFrame();
  void Shutdown();

  bool ShouldShutdown() const { return should_shutdown_; }

private:
  bool InstallHook();
  void RemoveHook();

  void DrawSnapshot(const ChampionSnapshot &snapshot);

  struct Impl;
  std::unique_ptr<Impl> impl_;
  MemoryScanner &scanner_;
  bool should_shutdown_ = false;
};
