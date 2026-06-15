#include "characterArchive.h"
#include <cstring>
#include <filesystem>
#include <iostream>

// Entrada del índice: [char name[32]][uint64_t offset] = 40 bytes
static constexpr std::size_t CHAR_IDX_NAME_LEN = 32;
static constexpr std::size_t CHAR_IDX_ENTRY_SIZE =
    CHAR_IDX_NAME_LEN + sizeof(uint64_t);

CharacterArchive::CharacterArchive(const std::string &datPath,
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

void CharacterArchive::loadIndex() {
  std::ifstream f(indexPath_, std::ios::binary);
  if (!f.is_open())
    return;

  char nameBuf[CHAR_IDX_NAME_LEN];
  uint64_t offset = 0;

  while (f.read(nameBuf, CHAR_IDX_NAME_LEN) &&
         f.read(reinterpret_cast<char *>(&offset), sizeof(uint64_t))) {
    std::string name(nameBuf, strnlen(nameBuf, CHAR_IDX_NAME_LEN));
    index_[name] = offset;
  }
}

bool CharacterArchive::exists(const std::string &name) const {
  std::shared_lock lock(mutex_);
  return index_.count(name) > 0;
}

bool CharacterArchive::save(const std::string &name, const std::string &race,
                            const std::string &cls) {
  std::unique_lock lock(mutex_);

  if (index_.count(name)) {
    std::cerr << "[CharacterArchive] '" << name << "' ya existe." << std::endl;
    return false;
  }

  CharacterRecord rec;
  std::memset(&rec, 0, sizeof(rec));
  std::strncpy(rec.name, name.c_str(), sizeof(rec.name) - 1);
  std::strncpy(rec.race, race.c_str(), sizeof(rec.race) - 1);
  std::strncpy(rec.cls, cls.c_str(), sizeof(rec.cls) - 1);

  uint64_t offset =
      static_cast<uint64_t>(index_.size()) * sizeof(CharacterRecord);
  index_[name] = offset;

  // Escribir registro en dat
  std::fstream dat(datPath_, std::ios::binary | std::ios::in | std::ios::out);
  dat.seekp(static_cast<std::streamoff>(offset));
  dat.write(reinterpret_cast<const char *>(&rec), sizeof(rec));
  dat.flush();

  // Escribir entrada en índice
  std::ofstream idx(indexPath_, std::ios::binary | std::ios::app);
  char nameBuf[CHAR_IDX_NAME_LEN] = {};
  std::strncpy(nameBuf, name.c_str(), CHAR_IDX_NAME_LEN - 1);
  idx.write(nameBuf, CHAR_IDX_NAME_LEN);
  idx.write(reinterpret_cast<const char *>(&offset), sizeof(uint64_t));
  idx.flush();

  std::cout << "[CharacterArchive] Guardado: '" << name << "' raza='" << race
            << "' clase='" << cls << "'" << std::endl;
  return true;
}

std::optional<CharacterRecord>
CharacterArchive::load(const std::string &name) const {
  uint64_t offset = 0;
  {
    std::shared_lock lock(mutex_);
    auto it = index_.find(name);
    if (it == index_.end())
      return std::nullopt;
    offset = it->second;
  }

  std::ifstream dat(datPath_, std::ios::binary);
  if (!dat.is_open())
    return std::nullopt;

  dat.seekg(static_cast<std::streamoff>(offset));
  CharacterRecord rec;
  dat.read(reinterpret_cast<char *>(&rec), sizeof(rec));
  if (!dat)
    return std::nullopt;

  return rec;
}