#include "gameWorld.h"
#include "../game/clan/clanManager.h"

GameWorld::GameWorld(const std::string &mapPath,
                     NpcFactory &npcFactory,
                     ItemRepository &itemRepo,
                     const toml::table &config,
                     ClanManager &clanManager)
    : mapData(MapSerializer::load(mapPath)),
      collision(mapData),
      occupancy(),
      formulas(config),
      npcManager(npcFactory, collision, mapData),
      itemRepo(itemRepo),
      clanManager(clanManager),
      bankRepo(),
      resurrectionSystem(),
      priestHandler(itemRepo, resurrectionSystem, mapData, config),
      merchantHandler(itemRepo, config),
      bankerHandler(bankRepo),
      cityDispatcher(priestHandler, merchantHandler, bankerHandler),
      players(),
      groundManager(),
      spawnManager(config, npcManager, collision, occupancy),
      tileSize(config["world"]["tile_size"].value_or(96)),
      npcRespawnDelayMs(config["npcs"]["respawn_ms"].value_or(5000.0f))
{
    spawnManager.loadSpawnPoints(this->mapData);
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
      npcRespawnDelayMs(config["npcs"]["respawn_ms"].value_or(5000.0f))
{
    spawnManager.loadSpawnPoints(this->mapData);
}

void GameWorld::addPlayer(Player player)
{
    loadInitialInventoryForPlayer(player);
    const uint32_t id = player.getId();
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
    auto it = players.find(id);

    if (it == players.end())
    {
        return false;
    }

    Player &p = it->second;

    if (p.isResurrecting())
    {
        return false;
    }

    // Movimiento fino en píxeles.
    // Si queda lento, subilo a 5 o 6. No vuelvas a mover por tile.

    float dx = 0.0f;
    float dy = 0.0f;

    switch (dir)
    {
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
    auto it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found: " + std::to_string(id));
    return it->second;
}

const Player &GameWorld::getPlayer(uint32_t id) const
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
int GameWorld::getPixelX(uint32_t id) const { return static_cast<int>(players.at(id).getPixelX()); }
int GameWorld::getPixelY(uint32_t id) const { return static_cast<int>(players.at(id).getPixelY()); }

void GameWorld::giveExperience(uint32_t playerId, uint32_t exp, float xpMultiplier)
{
    Player &p = getPlayer(playerId);
    uint32_t limit = formulas.calcExpLimit(p.getLevel());
    int16_t newMaxHp = formulas.calcMaxHp(p.getRace(), p.getCls(), p.getLevel() + 1);
    int16_t newMaxMana = formulas.calcMaxMana(p.getRace(), p.getCls(), p.getLevel() + 1);
    uint32_t finalExp = static_cast<uint32_t>(exp * xpMultiplier);
    p.addExperience(finalExp, limit, newMaxHp, newMaxMana);
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

        uint32_t killExp = formulas.calcExpOnKill(
            target.getMaxHp(),
            attacker.getLevel(),
            target.getLevel());

        giveExperience(attackerId, killExp);
    }

    const uint32_t victimGoldBefore = target.getGold();
    const uint32_t safeGold = formulas.calcMaxGold(target.getLevel());

    const uint32_t excessGold = target.die(safeGold);

    std::cout << "[PVP GOLD BEFORE DIE] victimId="
              << targetId
              << " killerId="
              << attackerId
              << " victimGoldBefore="
              << victimGoldBefore
              << " safeGold="
              << safeGold
              << " excessGold="
              << excessGold
              << " victimGoldAfter="
              << target.getGold()
              << std::endl;

    const int tileX = target.getTileX();
    const int tileY = target.getTileY();

    // El oro en exceso cae al piso, visible para cualquiera.
    uint32_t goldInstanceId = 0;
    if (excessGold > 0)
    {
        goldInstanceId = addGoldOnGround(excessGold, tileX, tileY);

        std::cout << "[PVP GOLD DROP] victimId="
                  << targetId
                  << " excessGold="
                  << excessGold
                  << " instanceId="
                  << goldInstanceId
                  << " tile=(" << tileX << "," << tileY << ")"
                  << std::endl;
    }

    std::vector<Item> items = target.purgeInventoryOnDeath();

    // Los items del muerto tambien caen al piso.
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

void GameWorld::spawnNpc(const std::string &typeName, int tileX, int tileY)
{
    if (!collision.isWalkable(tileX, tileY))
    {
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

    if (occupancy.isOccupied(tileX, tileY))
    {
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

void GameWorld::spawnMapNpcs()
{
    std::cout << "[WORLD NPC] spawnMapNpcs iniciado. map="
              << mapData.width()
              << "x"
              << mapData.height()
              << std::endl;

    int npcTilesFound = 0;
    int npcSpawned = 0;

    for (uint16_t y = 0; y < mapData.height(); ++y)
    {
        for (uint16_t x = 0; x < mapData.width(); ++x)
        {
            const Tile &tile = mapData.at(x, y);

            // Si el tile no tiene NPC configurado, no hacemos nada.
            if (tile.npc == NpcType::NONE)
            {
                continue;
            }

            ++npcTilesFound;

            // Evitamos spawnear NPCs no combatibles si el mapa los marca.
            if (!isSpawnable(tile.npc))
            {
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
            if (typeName.empty())
            {
                continue;
            }

            // Guardamos el punto base para respawns.
            spawnPoints.push_back({typeName, {x, y}});

            // Spawn inicial con desplazamiento aleatorio alrededor del punto base.
            bool spawned = false;

            for (int attempts = 0; attempts < 10; ++attempts)
            {
                const int dx = (std::rand() % 7) - 3;
                const int dy = (std::rand() % 7) - 3;

                const int tx = static_cast<int>(x) + dx;
                const int ty = static_cast<int>(y) + dy;

                if (collision.isWalkable(tx, ty) &&
                    !occupancy.isOccupied(tx, ty))
                {
                    spawnNpc(typeName, tx, ty);
                    spawned = true;
                    ++npcSpawned;
                    break;
                }
            }

            if (!spawned)
            {
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

            if (!collision.isWalkable(x, y))
                continue;
            if (occupancy.isOccupied(x, y))
                continue;

            spawnNpc(typeName, x, y);
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

    result.resurrectionStarted = std::move(pendingResurrectionStarts);
    pendingResurrectionStarts.clear();

    float deltaMs = deltaSeconds * 1000.0f;
    resurrectionSystem.tick(deltaMs, [this, &result](uint32_t pid, int tx, int ty)
                            {
                                Player &p = getPlayer(pid);
                                p.stopResurrection();
                                resurrectPlayer(pid, tx, ty);

                                result.playersResurrected.push_back({pid,
                                                                     static_cast<uint16_t>(p.getTileX()),
                                                                     static_cast<uint16_t>(p.getTileY())});  result.playersChanged.push_back(pid); });

    tickPlayers(deltaSeconds, result);
    tickNpcs(result);

    const auto respawnedNpcIds = npcManager.tickRespawns(deltaMs);
    if (!respawnedNpcIds.empty())
    {
        std::cout << "[GameWorld] respawnedNpcIds="
                  << respawnedNpcIds.size()
                  << std::endl;
    }

    for (uint32_t npcId : respawnedNpcIds)
    {

        Npc *npc = npcManager.findNpc(npcId);

        if (npc == nullptr)
        {
            continue;
        }

        const int tx = npc->getSpawnTileX();
        const int ty = npc->getSpawnTileY();

        // Si el spawn original está bloqueado, intentamos alrededor.
        bool placed = false;

        if (collision.isWalkable(tx, ty) && !occupancy.isOccupied(tx, ty))
        {
            occupancy.occupy(tx, ty, npcId);
            npc->respawn();
            placed = true;
        }
        else
        {
            for (int dx = -1; dx <= 1 && !placed; ++dx)
            {
                for (int dy = -1; dy <= 1 && !placed; ++dy)
                {
                    if (dx == 0 && dy == 0)
                    {
                        continue;
                    }

                    const int altX = tx + dx;
                    const int altY = ty + dy;

                    if (!collision.isWalkable(altX, altY))
                    {
                        continue;
                    }

                    if (occupancy.isOccupied(altX, altY))
                    {
                        continue;
                    }

                    occupancy.occupy(altX, altY, npcId);
                    npc->respawn();
                    npc->setTilePos(altX, altY);
                    placed = true;
                }
            }
        }

        if (!placed)
        {
            // Si no encontró lugar, lo mandamos otra vez a respawn corto.
            npcManager.startRespawn(npcId, npcRespawnDelayMs);
            continue;
        }

        result.spawnedNpcs.push_back({npc->getId(),
                                      npc->getType(),
                                      npc->getName(),
                                      static_cast<uint16_t>(npc->getTileX() * tileSize),
                                      static_cast<uint16_t>(npc->getTileY() * tileSize),
                                      static_cast<uint16_t>(npc->getHp()),
                                      static_cast<uint16_t>(npc->getMaxHp()),
                                      npc->isHostile()});

        std::cout << "[GameWorld] NPC respawn mismo id="
                  << npc->getId()
                  << " name="
                  << npc->getName()
                  << std::endl;
    }
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

        // PERF: solo notifica si HP o mana cambiaron realmente este tick.
        const bool changed = player.tick(hpGained, manaGained);
        if (changed)
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
    auto npcResult = npcManager.tick(players);

    for (auto &intent : npcResult.moveIntents)
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
                int j = std::rand() % (i + 1);
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
                continue; // Las 4 direcciones están bloqueadas, no se mueve este tick.
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

    for (auto &attack : npcResult.attacks)
    {
        // Buscamos al jugador atacado por el NPC.
        auto it = players.find(attack.targetPlayerId);

        // Si no existe, ignoramos el ataque.
        if (it == players.end())
        {
            continue;
        }

        Player &target = it->second;

        // Un NPC no debe seguir atacando a un jugador muerto/fantasma.
        if (!target.isAlive() || target.isGhost() || target.getHp() == 0)
        {
            continue;
        }

        target.takeDamage(attack.damage);

        // Registramos hit y cambio de stats para que GameLoop mande HUD actualizado.
        result.playerHits.push_back({attack.targetPlayerId, attack.damage});
        result.playersChanged.push_back(attack.targetPlayerId);

        if (npcManager.hasNpc(attack.npcId))
        {
            const Npc &attackerNpc = npcManager.getNpc(attack.npcId);
            const int dx = target.getTileX() - attackerNpc.getTileX();
            const int dy = target.getTileY() - attackerNpc.getTileY();

            Direction dir;
            if (std::abs(dx) > std::abs(dy))
                dir = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
            else
                dir = (dy > 0) ? Direction::DOWN : Direction::UP;

            result.npcAttacksForAnim.push_back({attack.npcId, dir});
        }

        if (!target.getClanName().empty())
        {
            result.clanAllyHits.push_back({target.getClanName(),
                                           target.getName(),
                                           attack.targetPlayerId});
        }

        // Si murió con este golpe, avisamos que murio
        if (target.getHp() == 0)
        {
            DeathResult deathResult = handlePlayerDeath(attack.targetPlayerId, 0);

            result.playersDied.push_back(attack.targetPlayerId);
            result.playerDeathsByNpc.push_back({attack.targetPlayerId, deathResult});

            // Marcamos stats cambiadas después de la muerte.
            result.playersChanged.push_back(attack.targetPlayerId);
        }
    }

    for (auto &death : npcResult.deaths)
    {
        // El NPC muerto libera el tile.
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

        // Se informa la muerte para que GameLoop mande vida 0 al cliente.
        result.npcDeaths.push_back(death);

        // El mismo NPC entra en estado RESPAWNING.
        npcManager.startRespawn(death.npcId, npcRespawnDelayMs);

        std::cout << "[GameWorld] NPC pasa a RESPAWNING id="
                  << death.npcId
                  << " respawnMs="
                  << npcRespawnDelayMs
                  << std::endl;
    }
}

void GameWorld::resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY)
{
    Player &p = getPlayer(id);
    occupancy.free(p.getTileX(), p.getTileY());

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

bool GameWorld::hasNpc(uint32_t npcId) const { return npcManager.hasNpc(npcId); }
bool GameWorld::hasPlayer(uint32_t playerId) const { return players.find(playerId) != players.end(); }

bool GameWorld::damageNpc(uint32_t npcId, int16_t damage, uint32_t attackerPlayerId)
{

    Npc *npc = npcManager.findNpc(npcId);
    if (npc == nullptr)
    {
        std::cout << "[GameWorld] damageNpc: NPC inexistente id="
                  << npcId
                  << std::endl;
        return false;
    }
    if (!npc->isAlive())
    {
        std::cout << "[GameWorld] damageNpc: NPC no activo id="
                  << npcId
                  << std::endl;
        return false;
    }

    // Guardamos la posición antes de aplicar daño.
    // Si muere, hay que liberar este tile.
    const int tileX = npc->getTileX();
    const int tileY = npc->getTileY();
    const int16_t hpBefore = npc->getHp();

    const bool damaged = npcManager.damageNpc(npcId, damage, attackerPlayerId);

    if (!damaged)
    {
        return false;
    }

    // Si murió, no lo borramos del NpcManager.
    // Lo dejamos en RESPAWNING y liberamos su tile.
    if (!npc->isAlive())
    {
        occupancy.free(tileX, tileY);

        npcManager.startRespawn(npcId, npcRespawnDelayMs);

        std::cout << "[GameWorld] NPC pasa a RESPAWNING id="
                  << npcId
                  << " respawnMs="
                  << npcRespawnDelayMs
                  << std::endl;
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
        player.getInventory().addItem(itemRepo.createItem("casco_hierro"));
        player.getInventory().addItem(itemRepo.createItem("gorro_calabaza"));
        player.getInventory().addItem(itemRepo.createItem("gorro_navidad"));
        player.getInventory().addItem(itemRepo.createItem("gorro_arlequin"));
        player.getInventory().addItem(itemRepo.createItem("sombrero_magico"));
        player.getInventory().addItem(itemRepo.createItem("capucha"));
        player.getInventory().addItem(itemRepo.createItem("espada_maldita"));
        player.getInventory().addItem(itemRepo.createItem("armadura_sombras"));
        player.getInventory().addItem(itemRepo.createItem("escudo_maldito"));
        player.getInventory().addItem(itemRepo.createItem("armadura_placas"));
        player.getInventory().addItem(itemRepo.createItem("escudo_tortuga"));
        player.getInventory().addItem(itemRepo.createItem("tunica_iniciado"));
        player.getInventory().addItem(itemRepo.createItem("tunica_azul"));
        player.getInventory().addItem(itemRepo.createItem("uniforme_argentino"));
        return;
    }
    if (className == "Mage")
    {
        player.getInventory().addItem(itemRepo.createItem("vara_fresno"));
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
        player.getInventory().addItem(itemRepo.createItem("escudo_tortuga"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        player.getInventory().addItem(itemRepo.createItem("capucha"));
        player.getInventory().addItem(itemRepo.createItem("pocion_mana"));
        player.getInventory().addItem(itemRepo.createItem("vara_fresno"));
        player.getInventory().addItem(itemRepo.createItem("hacha"));
        player.getInventory().addItem(itemRepo.createItem("martillo"));
        player.getInventory().addItem(itemRepo.createItem("baculo_nudoso"));
        player.getInventory().addItem(itemRepo.createItem("baculo_engarzado"));
        player.getInventory().addItem(itemRepo.createItem("arco_simple"));
        player.getInventory().addItem(itemRepo.createItem("arco_compuesto"));
        player.getInventory().addItem(itemRepo.createItem("armadura_cuero"));
        player.getInventory().addItem(itemRepo.createItem("tunica_azul"));
        player.getInventory().addItem(itemRepo.createItem("escudo_hierro"));
        player.getInventory().addItem(itemRepo.createItem("casco_hierro"));
        player.getInventory().addItem(itemRepo.createItem("sombrero_magico"));
        player.getInventory().addItem(itemRepo.createItem("uniforme_argentino"));
        return;
    }
    if (className == "Warrior")
    {

        player.getInventory().addItem(itemRepo.createItem("casco_hierro"));
        player.getInventory().addItem(itemRepo.createItem("gorro_calabaza"));
        player.getInventory().addItem(itemRepo.createItem("gorro_navidad"));
        player.getInventory().addItem(itemRepo.createItem("gorro_arlequin"));
        player.getInventory().addItem(itemRepo.createItem("sombrero_magico"));
        player.getInventory().addItem(itemRepo.createItem("capucha"));
        player.getInventory().addItem(itemRepo.createItem("espada_maldita"));
        player.getInventory().addItem(itemRepo.createItem("armadura_sombras"));
        player.getInventory().addItem(itemRepo.createItem("escudo_maldito"));
        player.getInventory().addItem(itemRepo.createItem("armadura_placas"));
        player.getInventory().addItem(itemRepo.createItem("escudo_tortuga"));
        player.getInventory().addItem(itemRepo.createItem("tunica_iniciado"));
        player.getInventory().addItem(itemRepo.createItem("tunica_azul"));
        player.getInventory().addItem(itemRepo.createItem("uniforme_argentino"));

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
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0)
                continue;
            int tx = tileX + dx, ty = tileY + dy;
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
    NpcType t = mapData.at(static_cast<uint16_t>(tileX),
                           static_cast<uint16_t>(tileY))
                    .npc;
    return t != NpcType::NONE ? std::optional<NpcType>(t) : std::nullopt;
}

NpcDropResult GameWorld::handleNpcDeath(uint32_t npcId, uint32_t killerPlayerId)
{
    NpcDropResult dropResult{};

    // Buscamos el NPC muerto.
    Npc *npc = npcManager.findNpc(npcId);
    if (npc == nullptr)
    {
        return dropResult;
    }
    if (npc->isRespawning())
    {
        return dropResult;
    }

    const int tileX = npc->getTileX();
    const int tileY = npc->getTileY();

    dropResult.tileX = tileX;
    dropResult.tileY = tileY;

    // Liberamos el tile ocupado por el NPC muerto.
    occupancy.free(tileX, tileY);

    // Decidimos que dropea segun la zona del NPC.
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

            std::cout << "[NPC DROP] npcId=" << npcId
                      << " GOLD=" << goldAmount
                      << " instanceId=" << goldInstanceId
                      << " tile=(" << tileX << "," << tileY << ")"
                      << std::endl;
        }
    }

    else if (!drop.empty())
    {
        Item item = itemRepo.createItem(drop);
        addItemOnGround(item, tileX, tileY);

        dropResult.hasItem = true;
        dropResult.droppedItem = item;

        std::cout << "[NPC DROP] npcId=" << npcId
                  << " item=" << drop
                  << " tile=(" << tileX << "," << tileY << ")"
                  << std::endl;
    }

    // El NPC NO se borra, conserva su ID y entra en RESPAWNING.
    npcManager.startRespawn(npcId, npcRespawnDelayMs);

    std::cout << "[GameWorld] NPC pasa a RESPAWNING id="
              << npcId
              << " respawnMs="
              << npcRespawnDelayMs
              << std::endl;

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
    auto playerClanInfo = clanManager.findClanInfoForMember(player.getName());
    if (!playerClanInfo)
        return 0; // No tiene clan, no tiene aliados cerca

    const std::string &playerClan = playerClanInfo->first;
    int count = 0;

    for (const auto &[id, other] : players)
    {
        if (id == player.getId() || !other.isAlive())
            continue;

        auto otherClanInfo = clanManager.findClanInfoForMember(other.getName());
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

        auto otherClanInfo = clanManager.findClanInfoForMember(other.getName());
        if (otherClanInfo && otherClanInfo->first == clanName)
        {
            clanMembers.push_back(id);
        }
    }
    return clanMembers;
}