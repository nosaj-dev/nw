#include "memory_scanner.hpp"

#include <Psapi.h>
#include <TlHelp32.h>
#include <Windows.h>


#include <array>

namespace {

std::optional<uint32_t> FindProcessId(const std::string &process_name) {
  PROCESSENTRY32 entry{};
  entry.dwSize = sizeof(PROCESSENTRY32);

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    return std::nullopt;
  }

  std::optional<uint32_t> pid;
  if (Process32First(snapshot, &entry)) {
    do {
      if (_stricmp(entry.szExeFile, process_name.c_str()) == 0) {
        pid = entry.th32ProcessID;
        break;
      }
    } while (Process32Next(snapshot, &entry));
  }

  CloseHandle(snapshot);
  return pid;
}

} // namespace

void MemoryScanner::Reset() {
  if (process_handle_) {
    CloseHandle(process_handle_);
    process_handle_ = nullptr;
  }
  process_id_ = 0;
  module_base_ = 0;
  module_size_ = 0;
  local_player_ptr_ = 0;
  attack_range_offset_ = 0;
  spell_data_offset_ = 0;
}

bool MemoryScanner::Attach(const std::string &process_name) {
  Reset();

  auto pid = FindProcessId(process_name);
  if (!pid) {
    return false;
  }

  process_id_ = *pid;
  process_handle_ = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                                FALSE, process_id_);
  if (!process_handle_) {
    Reset();
    return false;
  }

  HMODULE modules[1024];
  DWORD bytes_needed = 0;
  if (!EnumProcessModules(process_handle_, modules, sizeof(modules),
                          &bytes_needed)) {
    Reset();
    return false;
  }

  module_base_ = reinterpret_cast<uintptr_t>(modules[0]);
  MODULEINFO module_info{};
  if (!GetModuleInformation(process_handle_, modules[0], &module_info,
                            sizeof(module_info))) {
    Reset();
    return false;
  }
  module_size_ = module_info.SizeOfImage;

  return RefreshOffsets();
}

void MemoryScanner::Detach() { Reset(); }

bool MemoryScanner::RefreshOffsets() {
  // TODO: Implement signature scanning for live offsets.
  // Placeholder values until reverse engineering yields real data.
  local_player_ptr_ = module_base_ + 0x123456;
  attack_range_offset_ = 0x200;
  spell_data_offset_ = 0x400;
  return true;
}

std::optional<ChampionSnapshot> MemoryScanner::SnapshotLocalChampion() const {
  if (!process_handle_ || !local_player_ptr_) {
    return std::nullopt;
  }

  auto local_player = Read<uintptr_t>(local_player_ptr_);
  if (!local_player) {
    return std::nullopt;
  }

  ChampionSnapshot snapshot;

  Read(local_player + 0x88, snapshot.x);
  Read(local_player + 0x8C, snapshot.y);
  Read(local_player + 0x90, snapshot.z);
  Read(local_player + attack_range_offset_, snapshot.attack_range);

  for (size_t i = 0; i < std::size(snapshot.spells); ++i) {
    SpellData spell;
    Read(local_player + spell_data_offset_ +
             static_cast<uintptr_t>(i) * sizeof(SpellData),
         spell);
    snapshot.spells[i] = spell;
  }

  return snapshot;
}

template <typename T>
bool MemoryScanner::Read(uintptr_t address, T &out) const {
  SIZE_T bytes_read = 0;
  if (!ReadProcessMemory(process_handle_, reinterpret_cast<LPCVOID>(address),
                         &out, sizeof(T), &bytes_read)) {
    return false;
  }
  return bytes_read == sizeof(T);
}

uintptr_t
MemoryScanner::FollowPointerChain(uintptr_t base,
                                  const std::vector<uintptr_t> &offsets) const {
  uintptr_t address = base;
  for (auto offset : offsets) {
    if (!Read(address, address)) {
      return 0;
    }
    address += offset;
  }
  return address;
}

uintptr_t MemoryScanner::FindPattern(const char *signature,
                                     const char *mask) const {
  const auto pattern_length = std::strlen(mask);
  std::vector<uint8_t> buffer(module_size_);

  SIZE_T bytes_read = 0;
  if (!ReadProcessMemory(process_handle_,
                         reinterpret_cast<LPCVOID>(module_base_), buffer.data(),
                         buffer.size(), &bytes_read)) {
    return 0;
  }

  for (size_t i = 0; i <= bytes_read - pattern_length; ++i) {
    bool match = true;
    for (size_t j = 0; j < pattern_length; ++j) {
      if (mask[j] == '?') {
        continue;
      }
      if (signature[j] != static_cast<char>(buffer[i + j])) {
        match = false;
        break;
      }
    }
    if (match) {
      return module_base_ + i;
    }
  }

  return 0;
}
