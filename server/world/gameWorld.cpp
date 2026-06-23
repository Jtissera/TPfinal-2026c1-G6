#include "gameWorld.h"

GameWorld::GameWorld(const std::string &mapPath,
                     NpcFactory &npcFactory,
                     ItemRepository &itemRepo,
                     const toml::table &config,
                     ClanManager &clanManager)
    : GameWorld(MapSerializer::load(mapPath), npcFactory, itemRepo, config, clanManager)
{
}

GameWorld::GameWorld(MapData mapData,
                     NpcFactory &npcFactory,
                     ItemRepository &itemRepo,
                     const toml::table &config,
                     ClanManager &clanManager)
    : mapData(std::move(mapData)),
      collision(this->mapData),
      occupancy(),
      formulas(config),
      npcManager(npcFactory, collision, this->mapData),
      itemRepo(itemRepo),
      clanManager(clanManager),
      bankRepo(),
      resurrectionSystem(),
      priestHandler(itemRepo, resurrectionSystem, this->mapData, config),
      merchantHandler(itemRepo, config),
      bankerHandler(bankRepo),
      cityDispatcher(priestHandler, merchantHandler, bankerHandler),
      players(),
      groundManager(),
      spawnManager(config, npcManager, collision, occupancy),
      tileSize(config["world"]["tile_size"].value_or(96)),
      npcRespawnDelayMs(config["npc"]["respawn_ms"].value_or(5000.0f)),
      playerMoveStep(config["player"]["move_step"].value_or(8.0f))
{
    spawnManager.loadSpawnPoints(this->mapData);
}

std::optional<std::pair<int, int>> GameWorld::findAndOccupyAdjacentTile(
    int tileX, int tileY, uint32_t entityId)
{
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            if (dx == 0 && dy == 0)
                continue;

            const int tx = tileX + dx;
            const int ty = tileY + dy;

            if (collision.isWalkable(tx, ty) && occupancy.occupy(tx, ty, entityId))
            {
                return std::make_pair(tx, ty);
            }
        }
    }
    return std::nullopt;
}

void GameWorld::addPlayer(Player player)
{
    loadInitialInventoryForPlayer(player);
    const uint32_t id = player.getId();
    const int tx = player.getTileX();
    const int ty = player.getTileY();

    if (occupancy.occupy(tx, ty, id))
    {
        players.emplace(id, std::move(player));
        return;
    }

    const std::optional<std::pair<int, int>> freeTile = findAndOccupyAdjacentTile(tx, ty, id);
    if (!freeTile)
    {
        throw std::runtime_error("No free tile near spawn for player");
    }

    player.setTilePos(freeTile->first, freeTile->second);
    players.emplace(id, std::move(player));
}

std::optional<Player> GameWorld::removePlayer(uint32_t id)
{
    std::unordered_map<uint32_t, Player>::iterator it = players.find(id);
    if (it == players.end())
        return std::nullopt;

    occupancy.free(it->second.getTileX(), it->second.getTileY());
    Player player = std::move(it->second);
    players.erase(it);
    return player;
}

bool GameWorld::movePlayer(uint32_t id, Direction dir)
{
    std::unordered_map<uint32_t, Player>::iterator it = players.find(id);

    if (it == players.end())
    {
        return false;
    }

    Player &p = it->second;

    if (p.isResurrecting())
    {
        return false;
    }

    float dx = 0.0f;
    float dy = 0.0f;

    switch (dir)
    {
    case Direction::UP:
        dy = -playerMoveStep;
        break;

    case Direction::DOWN:
        dy = playerMoveStep;
        break;

    case Direction::LEFT:
        dx = -playerMoveStep;
        break;

    case Direction::RIGHT:
        dx = playerMoveStep;
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

    if (!collision.isWalkable(newTileX, newTileY))
    {
        return false;
    }

    if (oldTileX != newTileX || oldTileY != newTileY)
    {
        if (!occupancy.move(oldTileX, oldTileY, newTileX, newTileY, id))
        {
            return false;
        }
    }

    p.setPixelPos(nextX, nextY);

    return true;
}

Player &GameWorld::getPlayer(uint32_t id)
{
    std::unordered_map<uint32_t, Player>::iterator it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found: " + std::to_string(id));
    return it->second;
}

const Player &GameWorld::getPlayer(uint32_t id) const
{
    std::unordered_map<uint32_t, Player>::const_iterator it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found: " + std::to_string(id));
    return it->second;
}

bool GameWorld::canPlayerAct(uint32_t id) const
{
    std::unordered_map<uint32_t, Player>::const_iterator it = players.find(id);
    if (it == players.end())
        return false;
    return it->second.isAlive();
}

int GameWorld::getTileX(uint32_t id) const { return players.at(id).getTileX(); }
int GameWorld::getTileY(uint32_t id) const { return players.at(id).getTileY(); }
int GameWorld::getPixelX(uint32_t id) const { return static_cast<int>(players.at(id).getPixelX()); }
int GameWorld::getPixelY(uint32_t id) const { return static_cast<int>(players.at(id).getPixelY()); }

void GameWorld::giveExperience(uint32_t playerId, uint32_t exp, float xpMultiplier)
{
    Player &p = getPlayer(playerId);
    const uint32_t finalExp = static_cast<uint32_t>(exp * xpMultiplier);

    uint32_t limit = formulas.calcExpLimit(p.getLevel());
    int16_t newMaxHp = formulas.calcMaxHp(p.getRace(), p.getCls(), p.getLevel() + 1);
    int16_t newMaxMana = formulas.calcMaxMana(p.getRace(), p.getCls(), p.getLevel() + 1);
    p.addExperience(finalExp, limit, newMaxHp, newMaxMana);

    while (p.getExp() >= formulas.calcExpLimit(p.getLevel()))
    {
        limit = formulas.calcExpLimit(p.getLevel());
        newMaxHp = formulas.calcMaxHp(p.getRace(), p.getCls(), p.getLevel() + 1);
        newMaxMana = formulas.calcMaxMana(p.getRace(), p.getCls(), p.getLevel() + 1);
        p.addExperience(0, limit, newMaxHp, newMaxMana);
    }
}

GameWorld::DeathResult GameWorld::handlePlayerDeath(uint32_t targetId, uint32_t attackerId)
{
    Player &target = getPlayer(targetId);

    if (target.isGhost())
    {
        return {0, {}};
    }

    if (attackerId != 0)
    {
        Player &attacker = getPlayer(attackerId);

        const uint32_t killExp = formulas.calcExpOnKill(
            target.getMaxHp(),
            attacker.getLevel(),
            target.getLevel());

        giveExperience(attackerId, killExp);
    }

    const uint32_t safeGold = formulas.calcMaxGold(target.getLevel());
    const uint32_t excessGold = target.die(safeGold);

    const int tileX = target.getTileX();
    const int tileY = target.getTileY();

    uint32_t goldInstanceId = 0;
    if (excessGold > 0)
    {
        goldInstanceId = addGoldOnGround(excessGold, tileX, tileY);
    }

    std::vector<Item> items = target.purgeInventoryOnDeath();

    for (const Item &item : items)
    {
        addItemOnGround(item, tileX, tileY);
    }

    occupancy.free(tileX, tileY);

    DeathResult result;
    result.excessGold = excessGold;
    result.goldInstanceId = goldInstanceId;
    result.droppedItems = std::move(items);
    result.tileX = tileX;
    result.tileY = tileY;

    return result;
}

void GameWorld::addItemOnGround(Item item, int tileX, int tileY)
{
    groundManager.addItem(std::move(item), tileX, tileY);
}

uint32_t GameWorld::addGoldOnGround(uint32_t amount, int tileX, int tileY)
{
    return groundManager.addGold(amount, tileX, tileY);
}

std::optional<Item> GameWorld::pickItemById(uint32_t instanceId)
{
    return groundManager.pickItemById(instanceId);
}

std::optional<uint32_t> GameWorld::pickGoldById(uint32_t instanceId)
{
    return groundManager.pickGoldById(instanceId);
}

const GroundManager &GameWorld::getGroundManager() const { return groundManager; }
const MapData &GameWorld::getMapData() const { return mapData; }

void GameWorld::spawnNpc(const std::string &typeName, int tileX, int tileY)
{
    if (!collision.isWalkable(tileX, tileY) || occupancy.isOccupied(tileX, tileY))
    {
        return;
    }

    const uint32_t npcId = npcManager.spawnNpc(typeName, tileX, tileY);
    occupancy.occupy(tileX, tileY, npcId);
}

void GameWorld::spawnMapNpcs()
{
    spawnCombatNpcsFromMap();
    spawnCityNpcsFromMap();
}

void GameWorld::spawnCombatNpcsFromMap()
{
    for (uint16_t y = 0; y < mapData.height(); ++y)
    {
        for (uint16_t x = 0; x < mapData.width(); ++x)
        {
            const Tile &tile = mapData.at(x, y);

            if (tile.npc == NpcType::NONE || !isSpawnable(tile.npc))
            {
                continue;
            }

            const std::string typeName = npcTypeKey(tile.npc);
            if (typeName.empty())
            {
                continue;
            }

            spawnPoints.push_back({typeName, {x, y}});

            for (int attempts = 0; attempts < 10; ++attempts)
            {
                const int dx = (std::rand() % 7) - 3;
                const int dy = (std::rand() % 7) - 3;

                const int tx = static_cast<int>(x) + dx;
                const int ty = static_cast<int>(y) + dy;

                if (collision.isWalkable(tx, ty) && !occupancy.isOccupied(tx, ty))
                {
                    spawnNpc(typeName, tx, ty);
                    break;
                }
            }
        }
    }
}

void GameWorld::spawnCityNpcsFromMap()
{
    for (uint16_t y = 0; y < mapData.height(); ++y)
    {
        for (uint16_t x = 0; x < mapData.width(); ++x)
        {
            const Tile &tile = mapData.at(x, y);

            if (tile.npc != NpcType::PRIEST &&
                tile.npc != NpcType::MERCHANT &&
                tile.npc != NpcType::BANKER)
                continue;

            const std::string typeName = npcTypeKey(tile.npc);
            if (typeName.empty())
                continue;

            if (!collision.isWalkable(x, y) || occupancy.isOccupied(x, y))
                continue;

            spawnNpc(typeName, x, y);
        }
    }
}

const std::unordered_map<uint32_t, Npc> &GameWorld::getNpcs() const
{
    return npcManager.getNpcs();
}

void GameWorld::handleResurrectionComplete(uint32_t playerId, int tileX, int tileY,
                                           WorldTickResult &result)
{
    Player &p = getPlayer(playerId);
    p.stopResurrection();
    resurrectPlayer(playerId, tileX, tileY);

    result.playersResurrected.push_back({playerId,
                                         static_cast<uint16_t>(p.getTileX()),
                                         static_cast<uint16_t>(p.getTileY())});
    result.playersChanged.push_back(playerId);
}

bool GameWorld::placeRespawnedNpc(Npc &npc, WorldTickResult &result)
{
    const int tx = npc.getSpawnTileX();
    const int ty = npc.getSpawnTileY();

    if (collision.isWalkable(tx, ty) && !occupancy.isOccupied(tx, ty))
    {
        occupancy.occupy(tx, ty, npc.getId());
        npc.respawn();
    }
    else
    {
        const std::optional<std::pair<int, int>> freeTile =
            findAndOccupyAdjacentTile(tx, ty, npc.getId());

        if (!freeTile)
        {
            return false;
        }

        npc.respawn();
        npc.setTilePos(freeTile->first, freeTile->second);
    }

    result.spawnedNpcs.push_back({npc.getId(),
                                  npc.getType(),
                                  npc.getName(),
                                  static_cast<uint16_t>(npc.getTileX() * tileSize),
                                  static_cast<uint16_t>(npc.getTileY() * tileSize),
                                  static_cast<uint16_t>(npc.getHp()),
                                  static_cast<uint16_t>(npc.getMaxHp()),
                                  static_cast<uint16_t>(npc.getStats().level),
                                  npc.isHostile()});
    return true;
}

void GameWorld::processNpcRespawns(float deltaMs, WorldTickResult &result)
{
    const std::vector<uint32_t> respawnedNpcIds = npcManager.tickRespawns(deltaMs);

    for (uint32_t npcId : respawnedNpcIds)
    {
        Npc *npc = npcManager.findNpc(npcId);
        if (npc == nullptr)
        {
            continue;
        }

        if (!placeRespawnedNpc(*npc, result))
        {
            npcManager.startRespawn(npcId, npcRespawnDelayMs);
        }
    }
}

GameWorld::WorldTickResult GameWorld::tick(float deltaSeconds)
{
    WorldTickResult result;

    result.resurrectionStarted = std::move(pendingResurrectionStarts);
    pendingResurrectionStarts.clear();

    const float deltaMs = deltaSeconds * 1000.0f;

    resurrectionSystem.tick(deltaMs, [this, &result](uint32_t pid, int tx, int ty)
                            { handleResurrectionComplete(pid, tx, ty, result); });

    tickPlayers(deltaSeconds, result);
    tickNpcs(result);
    processNpcRespawns(deltaMs, result);

    return result;
}

void GameWorld::tickPlayers(float deltaSeconds, WorldTickResult &result)
{
    for (auto &[id, player] : players)
    {
        if (!player.isAlive() && !player.isMeditating())
            continue;

        const float hpGained = formulas.calcHpRegen(player.getRace(), deltaSeconds);
        const float manaGained = player.isMeditating()
                                     ? formulas.calcManaRegenMeditating(player.getCls(), player.getRace(), deltaSeconds)
                                     : formulas.calcManaRegen(player.getRace(), deltaSeconds);

        if (player.tick(hpGained, manaGained))
            result.playersChanged.push_back(id);
    }

    for (auto &[id, player] : players)
    {
        if (player.isMeditating())
        {
            continue;
        }

        const Tile &tile = mapData.at(
            static_cast<uint16_t>(player.getTileX()),
            static_cast<uint16_t>(player.getTileY()));

        if (tile.type == TileType::DUNGEON_ENTRANCE ||
            tile.type == TileType::CAVERN_ENTRANCE)
        {
            if (!tile.targetMap.empty())
                result.instanceTransitions.push_back(
                    {id, tile.targetMap, player.getTileX(), player.getTileY()});
        }
        else if (tile.type == TileType::EXIT)
        {
            result.instanceTransitions.push_back(
                {id, "", player.getTileX(), player.getTileY()});
        }
    }
}

void GameWorld::tickNpcs(WorldTickResult &result)
{
    NpcTickResult npcResult = npcManager.tick(players);

    resolveNpcMovement(npcResult, result);
    resolveNpcAttacks(npcResult, result);
    resolveNpcDeaths(npcResult, result);
}

void GameWorld::resolveNpcMovement(NpcTickResult &npcResult, WorldTickResult &result)
{
    for (NpcMoveIntent &intent : npcResult.moveIntents)
    {
        int toX = intent.toX;
        int toY = intent.toY;

        if (!collision.isWalkable(toX, toY))
        {
            const int dx[4] = {1, -1, 0, 0};
            const int dy[4] = {0, 0, 1, -1};
            int order[4] = {0, 1, 2, 3};

            for (int i = 3; i > 0; --i)
            {
                const int j = std::rand() % (i + 1);
                std::swap(order[i], order[j]);
            }

            bool foundAlternative = false;
            for (int k = 0; k < 4; ++k)
            {
                const int idx = order[k];
                const int rx = intent.fromX + dx[idx];
                const int ry = intent.fromY + dy[idx];

                if (collision.isWalkable(rx, ry))
                {
                    toX = rx;
                    toY = ry;
                    foundAlternative = true;
                    break;
                }
            }

            if (!foundAlternative)
            {
                continue;
            }
        }

        const Npc &npc = npcManager.getNpcs().at(intent.npcId);
        if (!npcManager.isSameZone(toX, toY, npc.getStats().homeZone))
            continue;

        const Tile &destTile = mapData.at(static_cast<uint16_t>(toX), static_cast<uint16_t>(toY));
        if (destTile.zone == ZoneType::SAFE)
            continue;

        if (!occupancy.move(intent.fromX, intent.fromY, toX, toY, intent.npcId))
            continue;

        npcManager.applyMove(intent.npcId, toX, toY);
        result.npcsMoved.push_back(intent.npcId);
    }
}

void GameWorld::resolveNpcAttacks(NpcTickResult &npcResult, WorldTickResult &result)
{
    for (NpcAttack &attack : npcResult.attacks)
    {
        std::unordered_map<uint32_t, Player>::iterator it = players.find(attack.targetPlayerId);
        if (it == players.end())
        {
            continue;
        }

        Player &target = it->second;

        if (!target.isAlive() || target.isGhost() || target.getHp() == 0)
        {
            continue;
        }

        target.takeDamage(attack.damage);

        result.playerHits.push_back({attack.targetPlayerId, attack.damage});
        result.playersChanged.push_back(attack.targetPlayerId);

        if (npcManager.hasNpc(attack.npcId))
        {
            const Npc &attackerNpc = npcManager.getNpc(attack.npcId);
            const int dx = target.getTileX() - attackerNpc.getTileX();
            const int dy = target.getTileY() - attackerNpc.getTileY();

            const Direction dir = (std::abs(dx) > std::abs(dy))
                                      ? (dx > 0 ? Direction::RIGHT : Direction::LEFT)
                                      : (dy > 0 ? Direction::DOWN : Direction::UP);

            result.npcAttacksForAnim.push_back({attack.npcId, dir});
        }

        if (!target.getClanName().empty())
        {
            result.clanAllyHits.push_back({target.getClanName(),
                                           target.getName(),
                                           attack.targetPlayerId});
        }

        if (target.getHp() == 0)
        {
            DeathResult deathResult = handlePlayerDeath(attack.targetPlayerId, 0);

            result.playersDied.push_back(attack.targetPlayerId);
            result.playerDeathsByNpc.push_back({attack.targetPlayerId, deathResult});
            result.playersChanged.push_back(attack.targetPlayerId);
        }
    }
}

void GameWorld::resolveNpcDeaths(NpcTickResult &npcResult, WorldTickResult &result)
{
    for (NpcDeathResult &death : npcResult.deaths)
    {
        occupancy.free(death.tileX, death.tileY);

        if (death.goldDrop > 0)
        {
            groundManager.addGold(death.goldDrop, death.tileX, death.tileY);
        }

        if (!death.itemDrop.empty())
        {
            try
            {
                groundManager.addItem(itemRepo.createItem(death.itemDrop),
                                      death.tileX, death.tileY);
            }
            catch (const std::exception &e)
            {
                std::cerr << "[GameWorld] item drop failed: " << e.what() << std::endl;
            }
        }

        result.npcDeaths.push_back(death);
        npcManager.startRespawn(death.npcId, npcRespawnDelayMs);
    }
}

void GameWorld::resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY)
{
    Player &p = getPlayer(id);
    occupancy.free(p.getTileX(), p.getTileY());

    if (occupancy.occupy(spawnTileX, spawnTileY, id))
    {
        p.resurrect(spawnTileX, spawnTileY);
        return;
    }

    const std::optional<std::pair<int, int>> freeTile =
        findAndOccupyAdjacentTile(spawnTileX, spawnTileY, id);

    if (freeTile)
    {
        p.resurrect(freeTile->first, freeTile->second);
    }
}

bool GameWorld::hasNpc(uint32_t npcId) const { return npcManager.hasNpc(npcId); }
bool GameWorld::hasPlayer(uint32_t playerId) const { return players.find(playerId) != players.end(); }

bool GameWorld::damageNpc(uint32_t npcId, int16_t damage, uint32_t attackerPlayerId)
{
    Npc *npc = npcManager.findNpc(npcId);
    if (npc == nullptr || !npc->isAlive())
    {
        return false;
    }

    const int tileX = npc->getTileX();
    const int tileY = npc->getTileY();

    const bool damaged = npcManager.damageNpc(npcId, damage, attackerPlayerId);

    if (!damaged)
    {
        return false;
    }

    if (!npc->isAlive())
    {
        occupancy.free(tileX, tileY);
        npcManager.startRespawn(npcId, npcRespawnDelayMs);
    }

    return true;
}

Npc &GameWorld::getNpc(uint32_t npcId) { return npcManager.getNpc(npcId); }
const Npc &GameWorld::getNpc(uint32_t npcId) const { return npcManager.getNpc(npcId); }

const std::unordered_map<uint32_t, Player> &GameWorld::getPlayers() const { return players; }

void GameWorld::loadInitialInventoryForPlayer(Player &player)
{
    if (player.hasReceivedInitialInventory())
    {
        return;
    }

    player.markInitialInventoryGiven();

    const std::string &className = player.getCls().name;

    if (className == "Cleric")
    {
        player.getInventory().addItem(itemRepo.createItem("capucha"));
        player.getInventory().addItem(itemRepo.createItem("tunica_iniciado"));
        player.getInventory().addItem(itemRepo.createItem("uniforme_argentino"));
        player.getInventory().addItem(itemRepo.createItem("vara_fresno"));
        return;
    }
    if (className == "Mage")
    {
        player.getInventory().addItem(itemRepo.createItem("baculo_nudoso"));
        player.getInventory().addItem(itemRepo.createItem("capucha"));
        player.getInventory().addItem(itemRepo.createItem("pocion_mana"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        player.getInventory().addItem(itemRepo.createItem("uniforme_argentino"));
        return;
    }
    if (className == "Paladin")
    {
        player.getInventory().addItem(itemRepo.createItem("espada"));
        player.getInventory().addItem(itemRepo.createItem("armadura_placas"));
        player.getInventory().addItem(itemRepo.createItem("baculo_nudoso"));
        player.getInventory().addItem(itemRepo.createItem("casco_hierro"));
        player.getInventory().addItem(itemRepo.createItem("uniforme_argentino"));
        return;
    }
    if (className == "Warrior")
    {
        player.getInventory().addItem(itemRepo.createItem("casco_hierro"));
        player.getInventory().addItem(itemRepo.createItem("armadura_placas"));
        player.getInventory().addItem(itemRepo.createItem("uniforme_argentino"));
        return;
    }

    player.getInventory().addItem(itemRepo.createItem("espada"));
    player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
}

const Tile &GameWorld::getTileAt(int tileX, int tileY) const
{
    return mapData.at(static_cast<uint16_t>(tileX), static_cast<uint16_t>(tileY));
}

std::pair<int, int> GameWorld::findSafeSpawnNear(int tileX, int tileY) const
{
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0)
                continue;
            const int tx = tileX + dx;
            const int ty = tileY + dy;
            if (!collision.isWalkable(tx, ty))
                continue;
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
    CityResult res = priestHandler.handleRemoteResurrect(player);

    if (res.ok && res.actionDelayMs > 0)
    {
        pendingResurrectionStarts.push_back({playerId, res.actionDelayMs});
    }
    return res;
}

std::optional<NpcType> GameWorld::getNpcTypeAtTile(int tileX, int tileY) const
{
    if (!collision.isInBounds(tileX, tileY))
        return std::nullopt;
    const NpcType t = mapData.at(static_cast<uint16_t>(tileX),
                                 static_cast<uint16_t>(tileY))
                          .npc;
    return t != NpcType::NONE ? std::optional<NpcType>(t) : std::nullopt;
}

NpcDropResult GameWorld::handleNpcDeath(uint32_t npcId, uint32_t killerPlayerId)
{
    NpcDropResult dropResult{};

    Npc *npc = npcManager.findNpc(npcId);
    if (npc == nullptr || npc->isRespawning())
    {
        return dropResult;
    }

    const int tileX = npc->getTileX();
    const int tileY = npc->getTileY();

    dropResult.tileX = tileX;
    dropResult.tileY = tileY;

    occupancy.free(tileX, tileY);

    const std::string drop = formulas.rollNpcDrop(npc->getStats().homeZone);

    if (drop == "GOLD")
    {
        const uint32_t goldAmount = formulas.calcNpcGoldDrop(npc->getMaxHp());

        if (goldAmount > 0)
        {
            const uint32_t goldInstanceId = addGoldOnGround(goldAmount, tileX, tileY);

            dropResult.hasGold = true;
            dropResult.goldInstanceId = goldInstanceId;
            dropResult.goldAmount = goldAmount;
        }
    }
    else if (!drop.empty())
    {
        Item item = itemRepo.createItem(drop);
        addItemOnGround(item, tileX, tileY);

        dropResult.hasItem = true;
        dropResult.droppedItem = item;
    }

    npcManager.startRespawn(npcId, npcRespawnDelayMs);

    return dropResult;
}

std::optional<uint32_t> GameWorld::findPlayerIdByName(const std::string &name) const
{
    for (const auto &[id, player] : players)
    {
        if (player.getName() == name)
            return id;
    }
    return std::nullopt;
}

int GameWorld::countClanAlliesNear(const Player &player, int radiusTiles) const
{
    const std::optional<std::pair<std::string, bool>> playerClanInfo =
        clanManager.findClanInfoForMember(player.getName());
    if (!playerClanInfo)
        return 0;

    const std::string &playerClan = playerClanInfo->first;
    int count = 0;

    for (const auto &[id, other] : players)
    {
        if (id == player.getId() || !other.isAlive())
            continue;

        const std::optional<std::pair<std::string, bool>> otherClanInfo =
            clanManager.findClanInfoForMember(other.getName());
        if (otherClanInfo && otherClanInfo->first == playerClan)
        {
            const int dx = std::abs(other.getTileX() - player.getTileX());
            const int dy = std::abs(other.getTileY() - player.getTileY());
            if (dx <= radiusTiles && dy <= radiusTiles)
                count++;
        }
    }
    return count;
}

std::vector<uint32_t> GameWorld::getOnlineClanMemberIds(const std::string &clanName) const
{
    std::vector<uint32_t> clanMembers;

    for (const auto &[id, other] : players)
    {
        if (!other.isAlive())
            continue;

        const std::optional<std::pair<std::string, bool>> otherClanInfo =
            clanManager.findClanInfoForMember(other.getName());
        if (otherClanInfo && otherClanInfo->first == clanName)
        {
            clanMembers.push_back(id);
        }
    }
    return clanMembers;
}