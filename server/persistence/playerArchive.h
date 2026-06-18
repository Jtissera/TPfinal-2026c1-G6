#pragma once

#include <cstdint>
#include <fstream>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "../game/items/itemRepository.h"
#include "../game/player/Player.h"
#include "../game/stats/classRepository.h"
#include "../game/stats/raceRepository.h"
#include "PlayerSnapshot.h"
#include "common/queue.h"
#include "common/thread.h"

class PlayerArchive : public Thread {
public:
  PlayerArchive(const std::string &datPath, const std::string &indexPath,
                ItemRepository &itemRepo, const RaceRepository &raceRepo,
                const ClassRepository &classRepo, const toml::table &config);

  ~PlayerArchive();

  void enqueue(PlayerSnapshot snap, uint32_t gameId);

  std::optional<Player> load(const std::string &name, uint32_t gameId);

  bool exists(const std::string &name) const;

  PlayerSnapshot toSnapshot(const Player &, const std::string &mapId,
                            uint32_t gameId) const;

  void run() override;
  void stop() override;

private:
  void loadIndex();
  void writeSnapshot(const PlayerSnapshot &snap);
  uint64_t allocateSlot(const std::string &name); // bajo lock exclusivo

  const std::string datPath_;
  const std::string indexPath_;

  ItemRepository &itemRepo;
  const RaceRepository &raceRepo;
  const ClassRepository &classRepo;
  const toml::table &config;

  std::fstream dat_;

  mutable std::shared_mutex indexMutex;
  std::unordered_map<std::string, uint64_t> index_;

  Queue<PlayerSnapshot> snapQueue;
};