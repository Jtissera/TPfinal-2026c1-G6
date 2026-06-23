#include "gameWorld.h"

#include <iostream>

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
      npcManager(npcFactory, collision, this->mapData, config),
      itemRepo(itemRepo),
      clanManager(clanManager),
      bankRepo(),
      resurrectionSystem(),
      priestHandler(itemRepo, resurrectionSystem, this->mapData, config),
      merchantHandler(itemRepo, config),
      bankerHandler(bankRepo),
      cityDispatcher(priestHandler, merchantHandler, bankerHandler),
      groundManager(),
      playerManager(occupancy, collision, formulas, itemRepo,
                    resurrectionSystem, clanManager, config),
      npcSpawner(npcManager, occupancy, collision, config),
      pendingResurrectionStarts()
{
    npcSpawner.spawnMapNpcs(this->mapData);
}

void GameWorld::addPlayer(Player player)
{
    playerManager.addPlayer(std::move(player));
}

std::optional<Player> GameWorld::removePlayer(uint32_t id)
{
    return playerManager.removePlayer(id);
}

bool GameWorld::movePlayer(uint32_t id, Direction dir)
{
    return playerManager.movePlayer(id, dir);
}

Player &GameWorld::getPlayer(uint32_t id)             { return playerManager.getPlayer(id); }
const Player &GameWorld::getPlayer(uint32_t id) const { return playerManager.getPlayer(id); }
bool GameWorld::hasPlayer(uint32_t id) const          { return playerManager.hasPlayer(id); }
bool GameWorld::canPlayerAct(uint32_t id) const       { return playerManager.canPlayerAct(id); }
int GameWorld::getTileX(uint32_t id) const            { return playerManager.getTileX(id); }
int GameWorld::getTileY(uint32_t id) const            { return playerManager.getTileY(id); }
int GameWorld::getPixelX(uint32_t id) const           { return playerManager.getPixelX(id); }
int GameWorld::getPixelY(uint32_t id) const           { return playerManager.getPixelY(id); }

void GameWorld::giveExperience(uint32_t playerId, uint32_t exp, float xpMultiplier)
{
    playerManager.giveExperience(playerId, exp, xpMultiplier);
}

DeathResult GameWorld::handlePlayerDeath(uint32_t targetId, uint32_t attackerId)
{
    return playerManager.handlePlayerDeath(targetId, attackerId, groundManager);
}

std::optional<uint32_t> GameWorld::findPlayerIdByName(const std::string &name) const
{
    return playerManager.findPlayerIdByName(name);
}

int GameWorld::countClanAlliesNear(const Player &player, int radiusTiles) const
{
    return playerManager.countClanAlliesNear(player, radiusTiles);
}

std::vector<uint32_t> GameWorld::getOnlineClanMemberIds(const std::string &clanName) const
{
    return playerManager.getOnlineClanMemberIds(clanName);
}

const std::unordered_map<uint32_t, Player> &GameWorld::getPlayers() const
{
    return playerManager.getPlayers();
}

void GameWorld::spawnMapNpcs()
{
    npcSpawner.spawnMapNpcs(mapData);
}

void GameWorld::spawnNpc(const std::string &typeName, int tileX, int tileY)
{
    npcSpawner.spawnNpc(typeName, tileX, tileY);
}

bool GameWorld::hasNpc(uint32_t npcId) const          { return npcManager.hasNpc(npcId); }
Npc &GameWorld::getNpc(uint32_t npcId)                { return npcManager.getNpc(npcId); }
const Npc &GameWorld::getNpc(uint32_t npcId) const    { return npcManager.getNpc(npcId); }

const std::unordered_map<uint32_t, Npc> &GameWorld::getNpcs() const
{
    return npcManager.getNpcs();
}

std::optional<NpcType> GameWorld::getNpcTypeAtTile(int tileX, int tileY) const
{
    if (!collision.isInBounds(tileX, tileY))
        return std::nullopt;

    const NpcType t = mapData.at(static_cast<uint16_t>(tileX),
                                  static_cast<uint16_t>(tileY)).npc;
    return t != NpcType::NONE ? std::optional<NpcType>(t) : std::nullopt;
}

bool GameWorld::damageNpc(uint32_t npcId, int16_t damage, uint32_t attackerPlayerId)
{
    Npc *npc = npcManager.findNpc(npcId);
    if (npc == nullptr || !npc->isAlive())
        return false;

    const int tileX = npc->getTileX();
    const int tileY = npc->getTileY();

    if (!npcManager.damageNpc(npcId, damage, attackerPlayerId))
        return false;

    if (!npc->isAlive())
    {
        occupancy.free(tileX, tileY);
        npcSpawner.startNpcRespawn(npcId);
    }

    return true;
}

NpcDropResult GameWorld::handleNpcDeath(uint32_t npcId, uint32_t killerPlayerId)
{
    NpcDropResult dropResult{};

    Npc *npc = npcManager.findNpc(npcId);
    if (npc == nullptr || npc->isRespawning())
        return dropResult;

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
            const uint32_t goldInstanceId = groundManager.addGold(goldAmount, tileX, tileY);
            dropResult.hasGold = true;
            dropResult.goldInstanceId = goldInstanceId;
            dropResult.goldAmount = goldAmount;
        }
    }
    else if (!drop.empty())
    {
        Item item = itemRepo.createItem(drop);
        groundManager.addItem(item, tileX, tileY);
        dropResult.hasItem = true;
        dropResult.droppedItem = item;
    }

    npcSpawner.startNpcRespawn(npcId);
    return dropResult;
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

const Tile &GameWorld::getTileAt(int tileX, int tileY) const
{
    return mapData.at(static_cast<uint16_t>(tileX), static_cast<uint16_t>(tileY));
}

std::pair<int, int> GameWorld::findSafeSpawnNear(int tileX, int tileY) const
{
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
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
        pendingResurrectionStarts.push_back({playerId, res.actionDelayMs});

    return res;
}

WorldTickResult GameWorld::tick(float deltaSeconds)
{
    WorldTickResult result;

    result.resurrectionStarted = std::move(pendingResurrectionStarts);
    pendingResurrectionStarts.clear();

    const float deltaMs = deltaSeconds * 1000.0f;

    resurrectionSystem.tick(deltaMs,
        [this, &result](uint32_t pid, int tx, int ty)
        {
            playerManager.handleResurrectionComplete(pid, tx, ty, result);
        });

    playerManager.tickPlayers(deltaSeconds, mapData, result);
    tickNpcs(result);
    npcSpawner.processRespawns(deltaMs, result);

    return result;
}

void GameWorld::tickNpcs(WorldTickResult &result)
{
    NpcTickResult npcResult = npcManager.tick(playerManager.getPlayers());

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
            const int offsets[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            int order[4] = {0, 1, 2, 3};

            for (int i = 3; i > 0; --i)
            {
                const int j = std::rand() % (i + 1);
                std::swap(order[i], order[j]);
            }

            bool foundAlternative = false;
            for (int k = 0; k < 4; ++k)
            {
                const int rx = intent.fromX + offsets[order[k]][0];
                const int ry = intent.fromY + offsets[order[k]][1];

                if (collision.isWalkable(rx, ry))
                {
                    toX = rx;
                    toY = ry;
                    foundAlternative = true;
                    break;
                }
            }

            if (!foundAlternative)
                continue;
        }

        const Npc &npc = npcManager.getNpcs().at(intent.npcId);
        if (!npcManager.isSameZone(toX, toY, npc.getStats().homeZone))
            continue;

        const Tile &destTile = mapData.at(static_cast<uint16_t>(toX),
                                           static_cast<uint16_t>(toY));
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
        if (!playerManager.hasPlayer(attack.targetPlayerId))
            continue;

        Player &target = playerManager.getPlayer(attack.targetPlayerId);

        if ((!target.isAlive() && !target.isMeditating()) || target.isGhost() || target.getHp() == 0)
            continue;
        
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
            groundManager.addGold(death.goldDrop, death.tileX, death.tileY);

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
        npcSpawner.startNpcRespawn(death.npcId);
    }
}

void GameWorld::resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY)
{
    playerManager.resurrectPlayer(id, spawnTileX, spawnTileY);
}