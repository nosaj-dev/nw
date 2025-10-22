#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct SpellData {
  float range = 0.0f;
  bool is_ready = false;
};

struct ChampionSnapshot {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float attack_range = 0.0f;
  SpellData spells[4]; // Q, W, E, R
};

class MemoryScanner {
public:
  MemoryScanner() = default;
  ~MemoryScanner() = default;

  bool Attach(const std::string &process_name);
  void Detach();

  bool RefreshOffsets();

  std::optional<ChampionSnapshot> SnapshotLocalChampion() const;

private:
  void Reset();

  template <typename T> bool Read(uintptr_t address, T &out) const;

  uintptr_t FollowPointerChain(uintptr_t base,
                               const std::vector<uintptr_t> &offsets) const;

  uintptr_t FindPattern(const char *signature, const char *mask) const;

  void *process_handle_ = nullptr;
  uint32_t process_id_ = 0;
  uintptr_t module_base_ = 0;
  uintptr_t module_size_ = 0;

  // Offsets discovered via reverse engineering (placeholder values until
  // populated)
  uintptr_t local_player_ptr_ = 0;
  uintptr_t attack_range_offset_ = 0;
  uintptr_t spell_data_offset_ = 0;
};
