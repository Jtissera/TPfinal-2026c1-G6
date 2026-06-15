#include "playerArchive.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

// ─── formato del index.dat
// ──────────────────────────────────────────────────── Entrada fija: [char
// key[48]][uint64_t offset] = 56 bytes key = "nombre@gameId"  (MAX_NAME_LEN=32
// + '@' + uint32 en decimal ≤ 10 chars)
static constexpr std::size_t INDEX_KEY_LEN = 48;
static constexpr std::size_t INDEX_ENTRY_SIZE =
    INDEX_KEY_LEN + sizeof(uint64_t);

// ─── helper ──────────────────────────────────────────────────────────────────

static std::string makeKey(const std::string &name, uint32_t gameId) {
  return name + "@" + std::to_string(gameId);
}

// ─── constructor / destructor
// ─────────────────────────────────────────────────

PlayerArchive::PlayerArchive(const std::string &datPath,
                             const std::string &indexPath,
                             ItemRepository &itemRepo,
                             const RaceRepository &raceRepo,
                             const ClassRepository &classRepo,
                             const toml::table &config)
    : datPath_(datPath), indexPath_(indexPath), itemRepo(itemRepo),
      raceRepo(raceRepo), classRepo(classRepo), config(config) {
  std::filesystem::path p(datPath_);
  if (p.has_parent_path())
    std::filesystem::create_directories(p.parent_path());

  if (!std::filesystem::exists(datPath_)) {
    std::ofstream newFile(datPath_, std::ios::binary);
    newFile.close();
  }

  dat_.open(datPath_, std::ios::binary | std::ios::in | std::ios::out);
  if (!dat_.is_open())
    throw std::runtime_error("[PlayerArchive] Error fatal al abrir/crear: " +
                             datPath_);

  dat_.clear();
  dat_.seekp(0);

  loadIndex();
}

PlayerArchive::~PlayerArchive() {
  if (dat_.is_open()) {
    dat_.flush();
    dat_.close();
  }
}

// ─── índice
// ───────────────────────────────────────────────────────────────────

void PlayerArchive::loadIndex() {
  std::ifstream f(indexPath_, std::ios::binary);
  if (!f.is_open())
    return;

  char keyBuf[INDEX_KEY_LEN];
  uint64_t offset = 0;

  while (f.read(keyBuf, INDEX_KEY_LEN) &&
         f.read(reinterpret_cast<char *>(&offset), sizeof(uint64_t))) {
    std::string key(keyBuf, strnlen(keyBuf, INDEX_KEY_LEN));
    index_[key] = offset;
  }
}

// PRECONDICIÓN: indexMutex ya tomado en modo exclusivo
uint64_t PlayerArchive::allocateSlot(const std::string &key) {
  uint64_t offset =
      static_cast<uint64_t>(index_.size()) * sizeof(PlayerSnapshot);
  index_[key] = offset;
  return offset;
}

// ─── snapshot
// ─────────────────────────────────────────────────────────────────

PlayerSnapshot PlayerArchive::toSnapshot(const Player &player,
                                         const std::string &mapId,
                                         uint32_t gameId) const {
  PlayerSnapshot snap;
  std::memset(&snap, 0, sizeof(snap));
  snap.version = SNAPSHOT_VERSION;

  std::strncpy(snap.name, player.getName().c_str(), sizeof(snap.name) - 1);
  std::strncpy(snap.mapId, mapId.c_str(), sizeof(snap.mapId) - 1);
  std::strncpy(snap.race, player.getRace().name.c_str(), sizeof(snap.race) - 1);
  std::strncpy(snap.cls, player.getCls().name.c_str(), sizeof(snap.cls) - 1);

  snap.pixelX = player.getPixelX();
  snap.pixelY = player.getPixelY();
  snap.hp = player.getHp();
  snap.maxHp = player.getMaxHp();
  snap.mana = player.getMana();
  snap.maxMana = player.getMaxMana();
  snap.level = player.getLevel();
  snap.gold = player.getGold();
  snap.experience = player.getExp();
  snap.gameId = gameId;

  const auto &invItems = player.getInventory().getItems();
  snap.itemCount =
      static_cast<uint8_t>(std::min(invItems.size(), std::size_t{MAX_ITEMS}));

  for (uint8_t i = 0; i < snap.itemCount; ++i) {
    const Item &src = invItems[i];
    ItemSnapshot &dst = snap.items[i];
    dst.catalogId = src.catalogId;
    dst.slot = static_cast<uint8_t>(src.slot);
    dst.effect = static_cast<uint8_t>(src.effect);
    dst.damageMin = src.stats.damageMin;
    dst.damageMax = src.stats.damageMax;
    dst.defenseMin = src.stats.defenseMin;
    dst.defenseMax = src.stats.defenseMax;
    dst.healAmount = src.stats.healAmount;
    dst.manaAmount = src.stats.manaAmount;
    dst.manaCost = src.stats.manaCost;
    dst.isRanged = src.stats.isRanged ? 1 : 0;
  }

  for (std::size_t i = 0; i < 4; ++i) {
    const Item *eq =
        player.getInventory().getEquipped(static_cast<EquipSlot>(i));
    if (!eq)
      continue;
    snap.equipped[i].catalogId = eq->catalogId;
    snap.equipped[i].slot = static_cast<uint8_t>(i);
  }

  std::cout << "[Archive] toSnapshot key='" << makeKey(player.getName(), gameId)
            << "' mapId='" << mapId << "'" << std::endl;
  return snap;
}

// ─── hilo worker
// ──────────────────────────────────────────────────────────────

void PlayerArchive::enqueue(PlayerSnapshot snap, uint32_t gameId) {
  snap.gameId = gameId;
  std::cout << "[Archive] enqueue key='"
            << makeKey(std::string(snap.name,
                                   strnlen(snap.name, sizeof(snap.name))),
                       gameId)
            << "'" << std::endl;
  snapQueue.try_push(std::move(snap));
}

void PlayerArchive::run() {
  try {
    while (true) {
      PlayerSnapshot snap = snapQueue.pop();
      writeSnapshot(snap);
    }
  } catch (const ClosedQueue &) {
    PlayerSnapshot snap;
    while (snapQueue.try_pop(snap))
      writeSnapshot(snap);
    dat_.flush();
  } catch (const std::exception &e) {
    std::cerr << "[PlayerArchive] error: " << e.what() << std::endl;
  }
}

void PlayerArchive::stop() {
  Thread::stop();
  snapQueue.close();
}

// ─── escritura
// ────────────────────────────────────────────────────────────────

void PlayerArchive::writeSnapshot(const PlayerSnapshot &snap) {
  std::string name(snap.name, strnlen(snap.name, sizeof(snap.name)));

  if (name.empty()) {
    std::cerr << "[Archive] ERROR: snapshot con nombre vacío, descartando"
              << std::endl;
    return;
  }

  std::string key = makeKey(name, snap.gameId);

  if (key.size() >= INDEX_KEY_LEN) {
    std::cerr << "[Archive] ERROR: clave '" << key << "' supera los "
              << INDEX_KEY_LEN - 1 << " caracteres, descartando" << std::endl;
    return;
  }

  uint64_t offset = 0;
  bool isNew = false;

  {
    std::unique_lock lock(indexMutex);
    auto it = index_.find(key);
    if (it != index_.end()) {
      offset = it->second;
    } else {
      offset = allocateSlot(key);
      isNew = true;
    }
  }

  std::cout << "[Archive] write key='" << key << "' offset=" << offset
            << std::endl;

  dat_.seekp(static_cast<std::streamoff>(offset));
  dat_.write(reinterpret_cast<const char *>(&snap), sizeof(PlayerSnapshot));
  dat_.flush();

  if (!dat_)
    std::cerr << "[Archive] ERROR: falló la escritura para '" << key << "'"
              << std::endl;

  if (isNew) {
    std::ofstream idx(indexPath_, std::ios::binary | std::ios::app);
    char keyBuf[INDEX_KEY_LEN] = {};
    std::strncpy(keyBuf, key.c_str(), INDEX_KEY_LEN - 1);
    idx.write(keyBuf, INDEX_KEY_LEN);
    idx.write(reinterpret_cast<const char *>(&offset), sizeof(uint64_t));
    idx.flush();
  }
}

// ─── lectura
// ──────────────────────────────────────────────────────────────────

std::optional<Player> PlayerArchive::load(const std::string &name,
                                          uint32_t gameId) {
  std::string key = makeKey(name, gameId);

  uint64_t offset = 0;
  {
    std::shared_lock lock(indexMutex);
    auto it = index_.find(key);
    if (it == index_.end()) {
      std::cout << "[Archive] No se encontró '" << key << "' — jugador nuevo."
                << std::endl;
      return std::nullopt;
    }
    offset = it->second;
  }

  std::ifstream reader(datPath_, std::ios::binary);
  if (!reader.is_open())
    return std::nullopt;

  reader.seekg(static_cast<std::streamoff>(offset));
  PlayerSnapshot snap;
  reader.read(reinterpret_cast<char *>(&snap), sizeof(PlayerSnapshot));
  if (!reader)
    return std::nullopt;

  std::string raceName(snap.race, strnlen(snap.race, sizeof(snap.race)));
  std::string clsName(snap.cls, strnlen(snap.cls, sizeof(snap.cls)));

  if (!raceRepo.exists(raceName) || !classRepo.exists(clsName)) {
    std::cerr << "[PlayerArchive] raza/clase inválida para: '" << key << "'"
              << std::endl;
    return std::nullopt;
  }

  const RaceStats &race = raceRepo.get(raceName);
  const ClassStats &cls = classRepo.get(clsName);

  Player player(0,
                std::string(snap.name, strnlen(snap.name, sizeof(snap.name))),
                race, cls, snap.maxHp, snap.maxMana, config);

  player.setPixelPos(snap.pixelX, snap.pixelY);
  player.setHp(snap.hp);
  player.setMana(snap.mana);
  player.setLevel(snap.level);
  player.setGold(snap.gold);
  player.setExp(snap.experience);

  player.markInitialInventoryGiven();

  if (player.getHp() <= 0)
    player.forceGhostState();

  for (uint8_t i = 0; i < snap.itemCount; ++i) {
    const ItemSnapshot &src = snap.items[i];
    if (src.catalogId == 0)
      continue;
    auto optItem = itemRepo.findByCatalogId(src.catalogId);
    if (!optItem) {
      std::cerr << "[PlayerArchive] catalogId desconocido: " << src.catalogId
                << std::endl;
      continue;
    }
    player.getInventory().addItem(std::move(*optItem));
  }

  for (std::size_t i = 0; i < 4; ++i) {
    const EquipSlotSnapshot &eq = snap.equipped[i];
    if (eq.catalogId == 0)
      continue;
    auto optItem = itemRepo.findByCatalogId(eq.catalogId);
    if (!optItem)
      continue;
    uint32_t instanceId = optItem->instanceId;
    player.getInventory().addItem(std::move(*optItem));
    player.getInventory().equipItem(instanceId);
  }

  std::cout << "[Archive::load] key='" << key << "' hp=" << player.getHp()
            << " pixelX=" << player.getPixelX()
            << " items=" << player.getInventory().getItems().size()
            << std::endl;

  return player;
}