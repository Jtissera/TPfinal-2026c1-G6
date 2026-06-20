#pragma once
#include <cstdint>
#include <cstring>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct GameRecord {
  uint32_t gameId = 0;
  uint8_t maxPlayers = 0;
  uint8_t _pad[3] = {};
  char gameName[64] = {};
  char mapPath[128] = {};
};
static_assert(sizeof(GameRecord) == 200, "GameRecord size mismatch");

class GameArchive {
public:
  GameArchive(const std::string &datPath, const std::string &indexPath);

  void save(uint32_t gameId, const std::string &gameName,
            const std::string &mapPath, uint8_t maxPlayers);

  std::vector<GameRecord> loadAll() const;

  uint32_t maxGameId() const;

private:
  void loadIndex();
  uint64_t allocateSlot(uint32_t gameId);

  std::string datPath_;
  std::string indexPath_;

  std::unordered_map<uint32_t, uint64_t> index_;
  mutable std::shared_mutex mutex_;
};