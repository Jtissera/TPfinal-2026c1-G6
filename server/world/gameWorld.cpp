#include "gameWorld.h"

GameWorld::GameWorld(const std::string &mapPath,
                     NpcFactory &npcFactory,
                     ItemRepository &itemRepo,
                     const toml::table &config)
    : mapData(MapSerializer::load(mapPath)),
      collision(mapData),
      occupancy(),
      formulas(config),
      npcManager(npcFactory, collision, mapData),
      itemRepo(itemRepo),
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
                     const toml::table &config)
    : mapData(std::move(mapData)),
      collision(this->mapData),
      occupancy(),
      formulas(config),
      npcManager(npcFactory, collision, this->mapData),
      itemRepo(itemRepo),
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

    // Guardamos el oro antes de morir para saber cuánto tenía realmente.
    const uint32_t victimGoldBefore = target.getGold();

    // Calculamos cuánto oro puede conservar el muerto según su nivel.
    const uint32_t safeGold = formulas.calcMaxGold(target.getLevel());

    // Procesamos la muerte.
    // Esta función deja al muerto con safeGold como máximo
    // y devuelve el oro excedente.
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

    // Según alcance actual, el oro en exceso va directo al killer.
    if (excessGold > 0 && attackerId != 0)
    {
        Player &attacker = getPlayer(attackerId);
        attacker.addGold(excessGold);

        std::cout << "[PVP GOLD] killerId="
                  << attackerId
                  << " victimId="
                  << targetId
                  << " excessGold="
                  << excessGold
                  << " killerGold="
                  << attacker.getGold()
                  << " victimGold="
                  << target.getGold()
                  << std::endl;
    }

    std::vector<Item> items = target.purgeInventoryOnDeath();

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

void GameWorld::spawnNpc(const std::string &typeName, int tileX, int tileY)
{
    const bool isFriendlyStatic = (typeName == "priest" ||
                                   typeName == "merchant" ||
                                   typeName == "banker");

    if (isFriendlyStatic)
    {
        if (!collision.isInBounds(tileX, tileY))
        {
            return;
        }
    }
    else
    {
        if (!collision.isWalkable(tileX, tileY))
        {
            std::cout << "[WORLD NPC] no spawn. tile no caminable typeName="
                      << typeName << " tile=(" << tileX << "," << tileY << ")" << std::endl;
            return;
        }
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

    float deltaMs = deltaSeconds * 1000.0f;
    resurrectionSystem.tick(deltaMs, [this](uint32_t pid, int tx, int ty)
                            {
        Player &p = getPlayer(pid);
        p.stopResurrection();
        resurrectPlayer(pid, tx, ty); });

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

        player.tick(hpGained, manaGained);
        result.playersChanged.push_back(id);
    }

    for (auto &[id, player] : players)
    {
        if (!player.isAlive() || player.isGhost() || player.getHp() == 0)
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
        if (!collision.isWalkable(intent.toX, intent.toY))
            continue;

        const Npc &npc = npcManager.getNpcs().at(intent.npcId);
        if (!npcManager.isSameZone(intent.toX, intent.toY, npc.getStats().homeZone))
            continue;

        if (!occupancy.move(intent.fromX, intent.fromY,
                            intent.toX, intent.toY, intent.npcId))
            continue;

        const Tile &destTile = mapData.at(
            static_cast<uint16_t>(intent.toX),
            static_cast<uint16_t>(intent.toY));
        if (destTile.zone == ZoneType::SAFE)
            continue;

        npcManager.applyMove(intent.npcId, intent.toX, intent.toY);
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

        // Si murió con este golpe, avisamos que murio
        if (target.getHp() == 0)
        {
            handlePlayerDeath(attack.targetPlayerId, 0);

            result.playersDied.push_back(attack.targetPlayerId);

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
        std::cout << "[GameWorld] damageNpc: npcManager.damageNpc devolvió false id="
                  << npcId
                  << std::endl;
        return false;
    }

    std::cout << "[GameWorld] damageNpc id="
              << npcId
              << " damage="
              << damage
              << " hp="
              << hpBefore
              << " -> "
              << npc->getHp()
              << std::endl;

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

    if (!player.getInventory().getItems().empty())
    {
        return;
    }

    const std::string &className = player.getCls().name;

    if (className == "Cleric")
    {
        player.getInventory().addItem(itemRepo.createItem("vara_fresno"));
        player.getInventory().addItem(itemRepo.createItem("capucha"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        player.getInventory().addItem(itemRepo.createItem("pocion_mana"));
        return;
    }
    if (className == "Mage")
    {
        player.getInventory().addItem(itemRepo.createItem("vara_fresno"));
        player.getInventory().addItem(itemRepo.createItem("capucha"));
        player.getInventory().addItem(itemRepo.createItem("pocion_mana"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
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
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        return;
    }
    if (className == "Warrior")
    {
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
    return priestHandler.handleRemoteResurrect(player);
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

void GameWorld::handleNpcDeath(uint32_t npcId, uint32_t killerPlayerId)
{
    // Buscamos el NPC muerto.
    Npc *npc = npcManager.findNpc(npcId);

    if (npc == nullptr)
    {
        return;
    }

    if (npc->isRespawning())
    {
        return;
    }
    std::cout << "[NPC GOLD DEBUG] npcId="
              << npcId
              << " killerPlayerId="
              << killerPlayerId
              << " npcMaxHp="
              << npc->getMaxHp()
              << std::endl;

    const int tileX = npc->getTileX();
    const int tileY = npc->getTileY();

    // Liberamos el tile ocupado por el NPC muerto.
    occupancy.free(tileX, tileY);

    // Si hay killer válido, calculamos drop directo.
    if (killerPlayerId != 0)
    {
        auto killerIt = players.find(killerPlayerId);

        if (killerIt != players.end())
        {
            Player &killer = killerIt->second;

            // Por ahora implementamos solo oro.
            const int roll = std::rand() % 100;

            if (roll >= 80 && roll < 88)
            {
                const double minFactor = 0.01;
                const double maxFactor = 0.20;

                const double random01 =
                    static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);

                const double factor =
                    minFactor + random01 * (maxFactor - minFactor);

                const uint32_t goldDrop = static_cast<uint32_t>(
                    factor * static_cast<double>(npc->getMaxHp()));

                if (goldDrop > 0)
                {
                    const uint32_t goldDrop = 100;
                    killer.addGold(goldDrop);

                    std::cout << "[NPC GOLD] killerId="
                              << killerPlayerId
                              << " npcId="
                              << npcId
                              << " goldDrop="
                              << goldDrop
                              << " killerGold="
                              << killer.getGold()
                              << std::endl;
                }
            }
        }
    }

    // El NPC NO se borra, conserva su ID y entra en RESPAWNING.
    npcManager.startRespawn(npcId, npcRespawnDelayMs);

    std::cout << "[GameWorld] NPC pasa a RESPAWNING id="
              << npcId
              << " respawnMs="
              << npcRespawnDelayMs
              << std::endl;
}