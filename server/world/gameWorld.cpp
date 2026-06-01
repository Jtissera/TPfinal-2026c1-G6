#include "gameWorld.h"

GameWorld::GameWorld(const std::string &mapPath, NpcFactory &npcFactory,
                     ItemRepository &itemRepo, const toml::table &config)
    : mapData(MapSerializer::load(mapPath)),
      collision(mapData),
      npcManager(npcFactory, collision, mapData),
      itemRepo(itemRepo),
      spawnManager(config, npcManager, collision, occupancy),
      tileSize(config["world"]["tile_size"].value_or(96))
{
  spawnManager.loadSpawnPoints(mapData);
}

GameWorld::GameWorld(MapData mapData, NpcFactory &npcFactory,
                     ItemRepository &itemRepo, const toml::table &config)
    : mapData(std::move(mapData)), collision(this->mapData),
      npcManager(npcFactory, collision, this->mapData), itemRepo(itemRepo),
      spawnManager(config, npcManager, collision, occupancy),
      tileSize(config["world"]["tile_size"].value_or(96))
{
  spawnManager.loadSpawnPoints(this->mapData);
}

// busca tile para spawnear y sino adyacentes

void GameWorld::addPlayer(Player player)
{
  uint32_t id = player.getId();
  int tx = player.getTileX();
  int ty = player.getTileY();

  if (!occupancy.occupy(tx, ty, id))
  {

    for (int dx = -1; dx <= 1; dx++)
    {
      for (int dy = -1; dy <= 1; dy++)
      {
        if (dx == 0 && dy == 0)
          continue;
        if (collision.isWalkable(tx + dx, ty + dy) &&
            occupancy.occupy(tx + dx, ty + dy, id))
        {
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

bool GameWorld::movePlayer(uint32_t id, Direction dir)
{
  const std::map<Direction, std::pair<int, int>> deltas = {
      {Direction::UP, {0, -1}},
      {Direction::DOWN, {0, 1}},
      {Direction::LEFT, {-1, 0}},
      {Direction::RIGHT, {1, 0}},
  };

  auto it = players.find(id);
  if (it == players.end())
    return false;

  auto deltaIt = deltas.find(dir);
  if (deltaIt == deltas.end())
    return false;

  Player &p = it->second;
  int tx = p.getTileX() + deltaIt->second.first;
  int ty = p.getTileY() + deltaIt->second.second;

  if (!collision.isWalkable(tx, ty))
    return false;
  if (!occupancy.move(p.getTileX(), p.getTileY(), tx, ty, id))
    return false;

  p.setTilePos(tx, ty);
  return true;
}

Player &GameWorld::getPlayer(uint32_t id)
{
  auto it = players.find(id);
  if (it == players.end())
    throw std::runtime_error("Player not found: " + std::to_string(id));
  return it->second;
}

bool GameWorld::canPlayerAct(uint32_t id) const
{
  auto it = players.find(id);
  if (it == players.end())
    return false;
  return it->second.isAlive();
}

int GameWorld::getTileX(uint32_t id) const { return players.at(id).getTileX(); }
int GameWorld::getTileY(uint32_t id) const { return players.at(id).getTileY(); }
int GameWorld::getPixelX(uint32_t id) const
{
  return players.at(id).getTileX() * tileSize;
}
int GameWorld::getPixelY(uint32_t id) const
{
  return players.at(id).getTileY() * tileSize;
}

void GameWorld::giveExperience(uint32_t playerId, uint32_t exp, float xpMultiplier)
{
  Player &p = getPlayer(playerId);
  uint32_t limit = formulas.calcExpLimit(p.getLevel());
  int16_t newMaxHp = formulas.calcMaxHp(p.getRace(), p.getCls(), p.getLevel() + 1);
  int16_t newMaxMana = formulas.calcMaxMana(p.getRace(), p.getCls(), p.getLevel() + 1);
  uint32_t finalExp = static_cast<uint32_t>(exp * xpMultiplier);
  p.addExperience(finalExp, limit, newMaxHp, newMaxMana);
}

GameWorld::DeathResult GameWorld::handlePlayerDeath(uint32_t targetId,
                                                    uint32_t attackerId)
{
  Player &target = getPlayer(targetId);

  // Exp al atacante si existe (0 = mató un NPC o muerte por otra causa)
  if (attackerId != 0)
  {
    Player &attacker = getPlayer(attackerId);
    uint32_t killExp = formulas.calcExpOnKill(
        target.getMaxHp(), attacker.getLevel(), target.getLevel());
    giveExperience(attackerId, killExp);
  }

  uint32_t safeGold = formulas.calcMaxGold(target.getLevel());
  uint32_t excessGold = target.die(safeGold);

  std::vector<Item> items = target.purgeInventoryOnDeath();

  if (excessGold > 0)
    addGoldOnGround(excessGold, target.getTileX(), target.getTileY());

  for (auto &item : items)
    addItemOnGround(std::move(item), target.getTileX(), target.getTileY());

  // Liberar el tile
  occupancy.free(target.getTileX(), target.getTileY());

  return {excessGold, std::move(items)};
}

void GameWorld::addItemOnGround(Item item, int tileX, int tileY)
{
  groundManager.addItem(std::move(item), tileX, tileY);
}

std::optional<Item> GameWorld::pickItemAt(int tileX, int tileY)
{
  return groundManager.pickItemAt(tileX, tileY);
}

void GameWorld::addGoldOnGround(uint32_t amount, int tileX, int tileY)
{
  groundManager.addGold(amount, tileX, tileY);
}

std::optional<uint32_t> GameWorld::pickGoldAt(int tileX, int tileY)
{
  return groundManager.pickGoldAt(tileX, tileY);
}

void GameWorld::spawnNpc(const std::string &typeName, int tileX, int tileY)
{
  spawnManager.spawnNpc(typeName, tileX, tileY);
}

const std::unordered_map<uint32_t, Npc> &GameWorld::getNpcs() const
{
  return npcManager.getNpcs();
}

GameWorld::WorldTickResult GameWorld::tick(float deltaSeconds)
{
  WorldTickResult result;

  tickPlayers(deltaSeconds, result);
  tickNpcs(result);
  spawnManager.tick();

  return result;
}

void GameWorld::tickPlayers(float deltaSeconds, WorldTickResult &result)
{
  for (auto &[id, player] : players)
  {
    if (!player.isAlive() && !player.isMeditating())
      continue;

    float hpGained = formulas.calcHpRegen(player.getRace(), deltaSeconds);
    float manaGained = player.isMeditating()
                           ? formulas.calcManaRegenMeditating(player.getCls(), player.getRace(), deltaSeconds)
                           : formulas.calcManaRegen(player.getRace(), deltaSeconds);

    player.tick(hpGained, manaGained);
    result.playersChanged.push_back(id);
  }

  for (auto &[id, player] : players)
  {
    if (!player.isAlive())
      continue;

    const Tile &tile = mapData.at(
        static_cast<uint16_t>(player.getTileX()),
        static_cast<uint16_t>(player.getTileY()));

    if (tile.type == TileType::DUNGEON_ENTRANCE ||
        tile.type == TileType::CAVERN_ENTRANCE)
    {
      if (!tile.targetMap.empty())
        result.instanceTransitions.push_back({id, tile.targetMap,
                                              player.getTileX(), player.getTileY()});
    }
    else if (tile.type == TileType::EXIT)
    {
      result.instanceTransitions.push_back({id, "", player.getTileX(), player.getTileY()});
    }
  }
}

void GameWorld::tickNpcs(WorldTickResult &result)
{
  auto npcResult = npcManager.tick(players);

  for (auto &intent : npcResult.moveIntents)
  {
    if (!collision.isWalkable(intent.toX, intent.toY))
      continue;

    const Npc &npc = npcManager.getNpcs().at(intent.npcId);
    if (!npcManager.isSameZone(intent.toX, intent.toY,
                               npc.getStats().homeZone))
      continue;

    if (!occupancy.move(intent.fromX, intent.fromY,
                        intent.toX, intent.toY, intent.npcId))
      continue;

    npcManager.applyMove(intent.npcId, intent.toX, intent.toY);
    result.npcsMoved.push_back(intent.npcId);
  }

  for (auto &attack : npcResult.attacks)
  {
    auto it = players.find(attack.targetPlayerId);
    if (it == players.end())
      continue;

    it->second.takeDamage(attack.damage);
    result.playerHits.push_back({attack.targetPlayerId, attack.damage});

    if (!it->second.isAlive())
      handlePlayerDeath(attack.targetPlayerId, 0);
  }

  for (auto &death : npcResult.deaths)
  {
    occupancy.free(death.tileX, death.tileY);

    if (death.goldDrop > 0)
      groundManager.addGold(death.goldDrop, death.tileX, death.tileY);

    if (!death.itemDrop.empty())
    {
      try
      {
        groundManager.addItem(itemRepo.createItem(death.itemDrop), death.tileX,
                              death.tileY);
      }
      catch (const std::exception &e)
      {
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

  // Buscar tile libre cerca del spawn
  if (occupancy.occupy(spawnTileX, spawnTileY, id))
  {
    p.resurrect(spawnTileX, spawnTileY);
  }
  else
  {
    for (int dx = -1; dx <= 1; dx++)
    {
      for (int dy = -1; dy <= 1; dy++)
      {
        if (dx == 0 && dy == 0)
          continue;
        int tx = spawnTileX + dx;
        int ty = spawnTileY + dy;
        if (collision.isWalkable(tx, ty) && occupancy.occupy(tx, ty, id))
        {
          p.resurrect(tx, ty);
          return;
        }
      }
    }
  }
}

const Tile &GameWorld::getTileAt(int tileX, int tileY) const
{
  return mapData.at(static_cast<uint16_t>(tileX),
                    static_cast<uint16_t>(tileY));
}

std::pair<int, int> GameWorld::findSafeSpawnNear(int tileX, int tileY) const
{
  for (int dx = -1; dx <= 1; dx++)
  {
    for (int dy = -1; dy <= 1; dy++)
    {
      if (dx == 0 && dy == 0)
        continue;
      int tx = tileX + dx;
      int ty = tileY + dy;
      if (!collision.isWalkable(tx, ty))
        continue;
      const Tile &t = mapData.at(
          static_cast<uint16_t>(tx),
          static_cast<uint16_t>(ty));
      if (t.type != TileType::DUNGEON_ENTRANCE &&
          t.type != TileType::CAVERN_ENTRANCE &&
          t.type != TileType::EXIT)
        return {tx, ty};
    }
  }
  return {tileX, tileY};
}