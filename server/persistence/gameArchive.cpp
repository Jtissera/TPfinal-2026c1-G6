#include "gameArchive.h"
#include <cstring>
#include <filesystem>
#include <iostream>

// Entrada del índice: [uint32_t gameId][uint64_t offset] = 12 bytes
static constexpr std::size_t IDX_ENTRY_SIZE =
    sizeof(uint32_t) + sizeof(uint64_t);

GameArchive::GameArchive(const std::string &datPath,
                         const std::string &indexPath)
    : datPath_(datPath), indexPath_(indexPath) {
  std::filesystem::path p(datPath_);
  if (p.has_parent_path())
    std::filesystem::create_directories(p.parent_path());

  if (!std::filesystem::exists(datPath_)) {
    std::ofstream f(datPath_, std::ios::binary);
  }
  if (!std::filesystem::exists(indexPath_)) {
    std::ofstream f(indexPath_, std::ios::binary);
  }

  loadIndex();
}

void GameArchive::loadIndex() {
  std::ifstream f(indexPath_, std::ios::binary);
  if (!f.is_open())
    return;

  uint32_t gameId = 0;
  uint64_t offset = 0;

  while (f.read(reinterpret_cast<char *>(&gameId), sizeof(uint32_t)) &&
         f.read(reinterpret_cast<char *>(&offset), sizeof(uint64_t))) {
    index_[gameId] = offset;
  }
}

uint64_t GameArchive::allocateSlot(uint32_t gameId) {
  uint64_t offset = static_cast<uint64_t>(index_.size()) * sizeof(GameRecord);
  index_[gameId] = offset;
  return offset;
}

void GameArchive::save(uint32_t gameId, const std::string &gameName,
                       const std::string &mapPath, uint8_t maxPlayers) {
  std::unique_lock lock(mutex_);

  if (index_.count(gameId))
    return;

  GameRecord rec;
  std::memset(&rec, 0, sizeof(rec));
  rec.gameId = gameId;
  rec.maxPlayers = maxPlayers;
  std::strncpy(rec.gameName, gameName.c_str(), sizeof(rec.gameName) - 1);
  std::strncpy(rec.mapPath, mapPath.c_str(), sizeof(rec.mapPath) - 1);

  uint64_t offset = allocateSlot(gameId);

  // Escribir en dat
  std::fstream dat(datPath_, std::ios::binary | std::ios::in | std::ios::out);
  dat.seekp(static_cast<std::streamoff>(offset));
  dat.write(reinterpret_cast<const char *>(&rec), sizeof(rec));
  dat.flush();

  // Escribir en índice
  std::ofstream idx(indexPath_, std::ios::binary | std::ios::app);
  idx.write(reinterpret_cast<const char *>(&gameId), sizeof(uint32_t));
  idx.write(reinterpret_cast<const char *>(&offset), sizeof(uint64_t));
  idx.flush();

  std::cout << "[GameArchive] guardada gameId=" << gameId << " name='"
            << gameName << "'" << std::endl;
}

std::vector<GameRecord> GameArchive::loadAll() const {
  std::shared_lock lock(mutex_);

  std::vector<GameRecord> result;
  if (index_.empty())
    return result;

  std::ifstream dat(datPath_, std::ios::binary);
  if (!dat.is_open())
    return result;

  for (const auto &[gameId, offset] : index_) {
    dat.seekg(static_cast<std::streamoff>(offset));
    GameRecord rec;
    dat.read(reinterpret_cast<char *>(&rec), sizeof(rec));
    if (dat)
      result.push_back(rec);
  }

  return result;
}

uint32_t GameArchive::maxGameId() const {
  std::shared_lock lock(mutex_);
  uint32_t max = 0;
  for (const auto &[id, _] : index_)
    if (id > max)
      max = id;
  return max;
}