#include "gameWorld.h"

GameWorld::GameWorld(const std::string &mapPath,
                     NpcFactory &npcFactory,
                     ItemRepository &itemRepo,
                     const toml::table &config)
    : mapData(MapSerializer::load(mapPath)),
        collision(mapData),
        occupancy(),
        npcManager(npcFactory, collision, mapData),
        itemRepo(itemRepo),
        formulas(config),
        spawnManager(config, npcManager, collision, occupancy),
        tileSize(config["world"]["tile_size"].value_or(96)),
        bankRepo(),
        resurrectionSystem(),
        priestHandler(itemRepo, resurrectionSystem, mapData, config),
        merchantHandler(itemRepo, config),
        bankerHandler(bankRepo),
        cityDispatcher(priestHandler, merchantHandler, bankerHandler)
        {
  spawnManager.loadSpawnPoints(this->mapData);
}

GameWorld::GameWorld(MapData mapData, NpcFactory &npcFactory,
                     ItemRepository &itemRepo, const toml::table &config)
    : mapData(std::move(mapData)),
        collision(this->mapData),
        occupancy(),
        npcManager(npcFactory, collision, this->mapData),
        itemRepo(itemRepo),
        formulas(config),
        spawnManager(config, npcManager, collision, occupancy),
        tileSize(config["world"]["tile_size"].value_or(96)),
        bankRepo(),
        resurrectionSystem(),
        priestHandler(itemRepo, resurrectionSystem, this->mapData, config),
        merchantHandler(itemRepo, config),
        bankerHandler(bankRepo),
        cityDispatcher(priestHandler, merchantHandler, bankerHandler)
{
  spawnManager.loadSpawnPoints(this->mapData);
}

void GameWorld::addPlayer(Player player) {
    loadInitialInventoryForPlayer(player);
    const uint32_t id = player.getId();
    int tx = player.getTileX();
    int ty = player.getTileY();

    if (!occupancy.occupy(tx, ty, id)) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                if (collision.isWalkable(tx + dx, ty + dy) &&
                    occupancy.occupy(tx + dx, ty + dy, id)) {
                    player.setTilePos(tx + dx, ty + dy);
                    players.emplace(id, std::move(player));
                    return;
                }
            }
        }
        throw std::runtime_error("No free tile near spawn for player");
    }
    players.emplace(id, std::move(player));
}

std::optional<Player> GameWorld::removePlayer(uint32_t id)
{
  auto it = players.find(id);
  if (it == players.end())
    return std::nullopt;

  occupancy.free(it->second.getTileX(), it->second.getTileY());
  Player player = std::move(it->second);
  players.erase(it);
  return player;
}

bool GameWorld::movePlayer(uint32_t id, Direction dir) {
    auto it = players.find(id);

    if (it == players.end()) {
        return false;
    }

    Player& p = it->second;

    if (p.isResurrecting()) {
        return false;
    }

    // Movimiento fino en píxeles.
    // Si queda lento, subilo a 5 o 6. No vuelvas a mover por tile.


    float dx = 0.0f;
    float dy = 0.0f;

    switch (dir) {
        case Direction::UP:
            dy = -PLAYER_MOVE_STEP;
            break;

        case Direction::DOWN:
            dy = PLAYER_MOVE_STEP;
            break;

        case Direction::LEFT:
            dx = -PLAYER_MOVE_STEP;
            break;

        case Direction::RIGHT:
            dx = PLAYER_MOVE_STEP;
            break;

        default:
            return false;
    }

    const float currentX = p.getPixelX();
    const float currentY = p.getPixelY();

    const float nextX = currentX + dx;
    const float nextY = currentY + dy;

    const int oldTileX = p.getTileX();
    const int oldTileY = p.getTileY();

    const int newTileX = static_cast<int>(nextX) / tileSize;
    const int newTileY = static_cast<int>(nextY) / tileSize;

    if (!collision.isWalkable(newTileX, newTileY)) {
        return false;
    }

    if (oldTileX != newTileX || oldTileY != newTileY) {
        if (!occupancy.move(oldTileX, oldTileY, newTileX, newTileY, id)) {
            return false;
        }
    }

    p.setPixelPos(nextX, nextY);

    return true;
}

Player& GameWorld::getPlayer(uint32_t id) {
    auto it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found: " + std::to_string(id));
    return it->second;
}

const Player& GameWorld::getPlayer(uint32_t id) const {
    auto it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found: " + std::to_string(id));
    return it->second;
}

bool GameWorld::canPlayerAct(uint32_t id) const {
    auto it = players.find(id);
    if (it == players.end()) return false;
    return it->second.isAlive();
}

int GameWorld::getTileX(uint32_t id)  const { return players.at(id).getTileX(); }
int GameWorld::getTileY(uint32_t id)  const { return players.at(id).getTileY(); }
int GameWorld::getPixelX(uint32_t id) const {return static_cast<int>(players.at(id).getPixelX());}
int GameWorld::getPixelY(uint32_t id) const {return static_cast<int>(players.at(id).getPixelY());}

void GameWorld::giveExperience(uint32_t playerId, uint32_t exp, float xpMultiplier)
{
  Player &p = getPlayer(playerId);
  uint32_t limit = formulas.calcExpLimit(p.getLevel());
  int16_t newMaxHp   = formulas.calcMaxHp(p.getRace(), p.getCls(), p.getLevel() + 1);
  int16_t newMaxMana = formulas.calcMaxMana(p.getRace(), p.getCls(), p.getLevel() + 1);
  uint32_t finalExp  = static_cast<uint32_t>(exp * xpMultiplier);
  p.addExperience(finalExp, limit, newMaxHp, newMaxMana);
}

GameWorld::DeathResult GameWorld::handlePlayerDeath(uint32_t targetId,
                                                     uint32_t attackerId) {
    Player& target = getPlayer(targetId);

    if (attackerId != 0) {
        Player &attacker = getPlayer(attackerId);
        uint32_t killExp = formulas.calcExpOnKill(
            target.getMaxHp(), attacker.getLevel(), target.getLevel());
        giveExperience(attackerId, killExp);
    }

    uint32_t safeGold  = formulas.calcMaxGold(target.getLevel());
    uint32_t excessGold = target.die(safeGold);
    std::vector<Item> items = target.purgeInventoryOnDeath();

    if (excessGold > 0)
        addGoldOnGround(excessGold, target.getTileX(), target.getTileY());

    for (auto &item : items)
        addItemOnGround(std::move(item), target.getTileX(), target.getTileY());

    occupancy.free(target.getTileX(), target.getTileY());

    return {excessGold, std::move(items)};
}

void GameWorld::addItemOnGround(Item item, int tileX, int tileY)
{
  groundManager.addItem(std::move(item), tileX, tileY);
}

void GameWorld::addGoldOnGround(uint32_t amount, int tileX, int tileY)
{
    groundManager.addGold(amount, tileX, tileY);
}

std::optional<Item> GameWorld::pickItemAt(int tileX, int tileY)
{
    return groundManager.pickItemAt(tileX, tileY);
}

std::optional<uint32_t> GameWorld::pickGoldAt(int tileX, int tileY)
{
    return groundManager.pickGoldAt(tileX, tileY);
}

void GameWorld::spawnNpc(const std::string& typeName, int tileX, int tileY) {
    if (!collision.isWalkable(tileX, tileY)) {
        std::cout << "[WORLD NPC] no spawn. tile no caminable typeName="
                  << typeName
                  << " tile=("
                  << tileX
                  << ","
                  << tileY
                  << ")"
                  << std::endl;
        return;
    }

    if (occupancy.isOccupied(tileX, tileY)) {
        std::cout << "[WORLD NPC] no spawn. tile ocupado typeName="
                  << typeName
                  << " tile=("
                  << tileX
                  << ","
                  << tileY
                  << ")"
                  << std::endl;
        return;
    }

    const uint32_t npcId = npcManager.spawnNpc(typeName, tileX, tileY);

    occupancy.occupy(tileX, tileY, npcId);

    std::cout << "[WORLD NPC] spawned npcId="
              << npcId
              << " typeName="
              << typeName
              << " tile=("
              << tileX
              << ","
              << tileY
              << ")"
              << std::endl;
}

void GameWorld::spawnMapNpcs() {
    std::cout << "[WORLD NPC] spawnMapNpcs iniciado. map="
              << mapData.width()
              << "x"
              << mapData.height()
              << std::endl;

    int npcTilesFound = 0;
    int npcSpawned = 0;

    for (uint16_t y = 0; y < mapData.height(); ++y) {
        for (uint16_t x = 0; x < mapData.width(); ++x) {
            const Tile& tile = mapData.at(x, y);

            // Si el tile no tiene NPC configurado, no hacemos nada.
            if (tile.npc == NpcType::NONE) {
                continue;
            }

            ++npcTilesFound;

            // Evitamos spawnear NPCs no combatibles si el mapa los marca.
            if (!isSpawnable(tile.npc)) {
                std::cout << "[WORLD NPC] npc no spawnable en x="
                          << x
                          << " y="
                          << y
                          << " npcType="
                          << static_cast<int>(tile.npc)
                          << std::endl;
                continue;
            }

            const std::string typeName = npcTypeKey(tile.npc);

            std::cout << "[WORLD NPC] tile con npc en x="
                      << x
                      << " y="
                      << y
                      << " npcType="
                      << static_cast<int>(tile.npc)
                      << " key="
                      << typeName
                      << std::endl;

            // Si no hay key válida, ignoramos el NPC.
            if (typeName.empty()) {
                continue;
            }

            // Guardamos el punto base para respawns.
            spawnPoints.push_back({typeName, {x, y}});

            // Spawn inicial con desplazamiento aleatorio alrededor del punto base.
            bool spawned = false;

            for (int attempts = 0; attempts < 10; ++attempts) {
                const int dx = (std::rand() % 7) - 3;
                const int dy = (std::rand() % 7) - 3;

                const int tx = static_cast<int>(x) + dx;
                const int ty = static_cast<int>(y) + dy;

                if (collision.isWalkable(tx, ty) &&
                    !occupancy.isOccupied(tx, ty)) {
                    spawnNpc(typeName, tx, ty);
                    spawned = true;
                    ++npcSpawned;
                    break;
                }
            }

            if (!spawned) {
                std::cout << "[WORLD NPC] no se pudo spawnear typeName="
                          << typeName
                          << " alrededor de x="
                          << x
                          << " y="
                          << y
                          << std::endl;
            }
        }
    }

    std::cout << "[WORLD NPC] spawnMapNpcs terminado. npcTilesFound="
              << npcTilesFound
              << " npcSpawned="
              << npcSpawned
              << " spawnPoints="
              << spawnPoints.size()
              << std::endl;
}

const std::unordered_map<uint32_t, Npc> &GameWorld::getNpcs() const
{
  return npcManager.getNpcs();
}

GameWorld::WorldTickResult GameWorld::tick(float deltaSeconds)
{
  WorldTickResult result;

  float deltaMs = deltaSeconds * 1000.0f;
  resurrectionSystem.tick(deltaMs, [this](uint32_t pid, int tx, int ty) {
        Player &p = getPlayer(pid);
        p.stopResurrection();
        resurrectPlayer(pid, tx, ty);
  });

  tickPlayers(deltaSeconds, result);
  tickNpcs(result);

  const auto spawnedNpcIds = spawnManager.tick();

  for (uint32_t npcId : spawnedNpcIds) {
        const Npc* npc = npcManager.findNpc(npcId);

        if (npc == nullptr) {
            continue;
        }
        result.spawnedNpcs.push_back({
        npc->getId(),
        npc->getType(),
        npc->getName(),
        static_cast<uint16_t>(npc->getTileX() * tileSize),
        static_cast<uint16_t>(npc->getTileY() * tileSize),
        static_cast<uint16_t>(npc->getHp()),
        static_cast<uint16_t>(npc->getMaxHp()),
        npc->isHostile()});
    }
return result;
}

void GameWorld::tickPlayers(float deltaSeconds, WorldTickResult &result)
{
  for (auto &[id, player] : players) {
    if (!player.isAlive() && !player.isMeditating()) continue;

    float hpGained   = formulas.calcHpRegen(player.getRace(), deltaSeconds);
    float manaGained = player.isMeditating()
        ? formulas.calcManaRegenMeditating(player.getCls(), player.getRace(), deltaSeconds)
        : formulas.calcManaRegen(player.getRace(), deltaSeconds);

    player.tick(hpGained, manaGained);
    result.playersChanged.push_back(id);
  }

  for (auto &[id, player] : players) {
    if (!player.isAlive()) continue;

    const Tile &tile = mapData.at(
        static_cast<uint16_t>(player.getTileX()),
        static_cast<uint16_t>(player.getTileY()));

    if (tile.type == TileType::DUNGEON_ENTRANCE ||
        tile.type == TileType::CAVERN_ENTRANCE) {
      if (!tile.targetMap.empty())
        result.instanceTransitions.push_back(
            {id, tile.targetMap, player.getTileX(), player.getTileY()});
    } else if (tile.type == TileType::EXIT) {
      result.instanceTransitions.push_back(
          {id, "", player.getTileX(), player.getTileY()});
    }
  }
}

void GameWorld::tickNpcs(WorldTickResult &result)
{
  auto npcResult = npcManager.tick(players);

  for (auto &intent : npcResult.moveIntents) {
    if (!collision.isWalkable(intent.toX, intent.toY)) continue;

    const Npc &npc = npcManager.getNpcs().at(intent.npcId);
    if (!npcManager.isSameZone(intent.toX, intent.toY, npc.getStats().homeZone))
      continue;

    if (!occupancy.move(intent.fromX, intent.fromY,
                        intent.toX, intent.toY, intent.npcId))
      continue;

    const Tile &destTile = mapData.at(
        static_cast<uint16_t>(intent.toX),
        static_cast<uint16_t>(intent.toY));
    if (destTile.zone == ZoneType::SAFE) continue;

    npcManager.applyMove(intent.npcId, intent.toX, intent.toY);
    result.npcsMoved.push_back(intent.npcId);
  }

  for (auto &attack : npcResult.attacks) {
    auto it = players.find(attack.targetPlayerId);
    if (it == players.end()) continue;

    it->second.takeDamage(attack.damage);
    result.playerHits.push_back({attack.targetPlayerId, attack.damage});

    if (!it->second.isAlive())
      handlePlayerDeath(attack.targetPlayerId, 0);
  }

  for (auto &death : npcResult.deaths) {
    occupancy.free(death.tileX, death.tileY);

    if (death.goldDrop > 0)
      groundManager.addGold(death.goldDrop, death.tileX, death.tileY);

    if (!death.itemDrop.empty()) {
      try {
        groundManager.addItem(itemRepo.createItem(death.itemDrop),
                              death.tileX, death.tileY);
      } catch (const std::exception &e) {
        std::cerr << "[GameWorld] item drop failed: " << e.what() << std::endl;
      }
    }

    result.npcDeaths.push_back(death);
  }
}

void GameWorld::resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY)
{
  Player &p = getPlayer(id);
  occupancy.free(p.getTileX(), p.getTileY());

  if (occupancy.occupy(spawnTileX, spawnTileY, id)) {
      p.resurrect(spawnTileX, spawnTileY);
  } else {
      for (int dx = -1; dx <= 1; dx++) {
          for (int dy = -1; dy <= 1; dy++) {
              if (dx == 0 && dy == 0) continue;
              int tx = spawnTileX + dx;
              int ty = spawnTileY + dy;
              if (collision.isWalkable(tx, ty) && occupancy.occupy(tx, ty, id)) {
                  p.resurrect(tx, ty);
                  return;
              }
          }
      }
  }
}

bool GameWorld::hasNpc(uint32_t npcId) const { return npcManager.hasNpc(npcId); }
bool GameWorld::hasPlayer(uint32_t playerId) const { return players.find(playerId) != players.end(); }

bool GameWorld::damageNpc(uint32_t npcId, int16_t damage, uint32_t attackerPlayerId) {
    return npcManager.damageNpc(npcId, damage, attackerPlayerId);
}

Npc& GameWorld::getNpc(uint32_t npcId) { return npcManager.getNpc(npcId); }
const Npc& GameWorld::getNpc(uint32_t npcId) const { return npcManager.getNpc(npcId); }

const std::unordered_map<uint32_t, Player>& GameWorld::getPlayers() const { return players; }

void GameWorld::loadInitialInventoryForPlayer(Player& player) {
    const std::string& className = player.getCls().name;

    if (className == "Cleric") {
        player.getInventory().addItem(itemRepo.createItem("vara_fresno"));
        player.getInventory().addItem(itemRepo.createItem("capucha"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        player.getInventory().addItem(itemRepo.createItem("pocion_mana"));
        return;
    }
    if (className == "Mage") {
        player.getInventory().addItem(itemRepo.createItem("vara_fresno"));
        player.getInventory().addItem(itemRepo.createItem("capucha"));
        player.getInventory().addItem(itemRepo.createItem("pocion_mana"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        return;
    }
    if (className == "Paladin") {
        player.getInventory().addItem(itemRepo.createItem("espada"));
        player.getInventory().addItem(itemRepo.createItem("armadura_placas"));
        player.getInventory().addItem(itemRepo.createItem("escudo_tortuga"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        player.getInventory().addItem(itemRepo.createItem("capucha"));
        player.getInventory().addItem(itemRepo.createItem("pocion_mana"));
        player.getInventory().addItem(itemRepo.createItem("vara_fresno"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        return;
    }
    if (className == "Warrior") {
        player.getInventory().addItem(itemRepo.createItem("espada"));
        player.getInventory().addItem(itemRepo.createItem("armadura_placas"));
        player.getInventory().addItem(itemRepo.createItem("escudo_tortuga"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        return;
    }

    std::cerr << "[GameWorld][Inventory] clase desconocida='" << className
              << "', cargando inventario default." << std::endl;
    player.getInventory().addItem(itemRepo.createItem("espada"));
    player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
}

const Tile &GameWorld::getTileAt(int tileX, int tileY) const
{
  return mapData.at(static_cast<uint16_t>(tileX), static_cast<uint16_t>(tileY));
}

std::pair<int, int> GameWorld::findSafeSpawnNear(int tileX, int tileY) const
{
  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      if (dx == 0 && dy == 0) continue;
      int tx = tileX + dx, ty = tileY + dy;
      if (!collision.isWalkable(tx, ty)) continue;
      const Tile &t = mapData.at(static_cast<uint16_t>(tx), static_cast<uint16_t>(ty));
      if (t.type != TileType::DUNGEON_ENTRANCE &&
          t.type != TileType::CAVERN_ENTRANCE &&
          t.type != TileType::EXIT)
        return {tx, ty};
    }
  }
  return {tileX, tileY};
}

CityResult GameWorld::handleCityInteraction(uint32_t playerId,
                                            NpcType npcType,
                                            const CityCommand &cmd)
{
  Player &player = getPlayer(playerId);
  return cityDispatcher.dispatch(npcType, cmd, player);
}

CityResult GameWorld::handleRemoteResurrect(uint32_t playerId)
{
  Player &player = getPlayer(playerId);
  return priestHandler.handleRemoteResurrect(player);
}

std::optional<NpcType> GameWorld::getNpcTypeAtTile(int tileX, int tileY) const
{
  if (!collision.isInBounds(tileX, tileY)) return std::nullopt;
  NpcType t = mapData.at(static_cast<uint16_t>(tileX),
                         static_cast<uint16_t>(tileY)).npc;
  return t != NpcType::NONE ? std::optional<NpcType>(t) : std::nullopt;
}

