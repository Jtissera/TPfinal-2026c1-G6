#include "gameWorld.h"

GameWorld::GameWorld(const std::string& mapPath,
                     NpcFactory& npcFactory,
                     ItemRepository& itemRepo,
                     const toml::table& config)
    : mapData(MapSerializer::load(mapPath))
    , collision(mapData)
    , npcManager(npcFactory, collision, mapData,
                 config["world"]["tile_size"].value_or(96))
    , itemRepo(itemRepo)
    , spawnManager(config, npcManager, collision)
    , tileSize(config["world"]["tile_size"].value_or(96))
    , playerHitboxW(config["world"]["player_hitbox_w"].value_or(32.0f))
    , playerHitboxH(config["world"]["player_hitbox_h"].value_or(16.0f))
    , npcHitboxW(config["world"]["npc_hitbox_w"].value_or(32.0f))
    , npcHitboxH(config["world"]["npc_hitbox_h"].value_or(16.0f))
    , adjacencyThreshold(config["world"]["adjacency_threshold"].value_or(144.0f))
    , bankRepo()
    , resurrectionSystem()
    , priestHandler(itemRepo, resurrectionSystem, mapData, config)
    , merchantHandler(itemRepo, config)
    , bankerHandler(bankRepo)
    , cityDispatcher(priestHandler, merchantHandler, bankerHandler)
{
    spawnManager.loadSpawnPoints(mapData);
}

GameWorld::GameWorld(MapData mapData,
                     NpcFactory& npcFactory,
                     ItemRepository& itemRepo,
                     const toml::table& config)
    : mapData(std::move(mapData))
    , collision(this->mapData)
    , npcManager(npcFactory, collision, this->mapData,
                 config["world"]["tile_size"].value_or(96))
    , itemRepo(itemRepo)
    , spawnManager(config, npcManager, collision)
    , tileSize(config["world"]["tile_size"].value_or(96))
    , playerHitboxW(config["world"]["player_hitbox_w"].value_or(32.0f))
    , playerHitboxH(config["world"]["player_hitbox_h"].value_or(16.0f))
    , npcHitboxW(config["world"]["npc_hitbox_w"].value_or(32.0f))
    , npcHitboxH(config["world"]["npc_hitbox_h"].value_or(16.0f))
    , adjacencyThreshold(config["world"]["adjacency_threshold"].value_or(144.0f))
    , bankRepo()
    , resurrectionSystem()
    , priestHandler(itemRepo, resurrectionSystem, this->mapData, config)
    , merchantHandler(itemRepo, config)
    , bankerHandler(bankRepo)
    , cityDispatcher(priestHandler, merchantHandler, bankerHandler)
{
    spawnManager.loadSpawnPoints(this->mapData);
}

// --- Helpers de hitbox ---

Rect GameWorld::playerHitbox(float px, float py) const {
    return {px - playerHitboxW / 2.0f,
            py - playerHitboxH,
            playerHitboxW,
            playerHitboxH};
}

Rect GameWorld::npcHitbox(float px, float py) const {
    return {px - npcHitboxW / 2.0f,
            py - npcHitboxH,
            npcHitboxW,
            npcHitboxH};
}

// Devuelve true si el hitbox centrado en (px,py) solapa con alguna entidad,
// excluyendo a excludeId (el que se esta moviendo).
bool GameWorld::entityCollides(float px, float py,
                                float hitW, float hitH,
                                uint32_t excludeId) const {
    Rect moving = {px - hitW / 2.0f, py - hitH, hitW, hitH};

    for (const auto& [id, player] : players) {
        if (id == excludeId) continue;
        if (!player.isAlive() && !player.isGhost()) continue;
        Rect other = playerHitbox(player.getPixelX(), player.getPixelY());
        if (collision.overlaps(moving, other)) return true;
    }

    for (const auto& [id, npc] : npcManager.getNpcs()) {
        if (id == excludeId) continue;
        if (!npc.isAlive()) continue;
        Rect other = npcHitbox(npc.getPixelX(), npc.getPixelY());
        if (collision.overlaps(moving, other)) return true;
    }

    return false;
}

// --- Players ---

void GameWorld::addPlayer(Player player) {
    uint32_t id  = player.getId();
    float px     = player.getPixelX();
    float py     = player.getPixelY();

    // Si la posicion inicial no es valida, busca una cercana libre
    if (!collision.isWalkable(px, py) ||
        entityCollides(px, py, playerHitboxW, playerHitboxH, id))
    {
        bool placed = false;
        for (int dx = -1; dx <= 1 && !placed; dx++) {
            for (int dy = -1; dy <= 1 && !placed; dy++) {
                if (dx == 0 && dy == 0) continue;
                float nx = px + dx * tileSize;
                float ny = py + dy * tileSize;
                if (collision.isWalkable(nx, ny) &&
                    !entityCollides(nx, ny, playerHitboxW, playerHitboxH, id))
                {
                    player.setPixelPos(nx, ny);
                    placed = true;
                }
            }
        }
        if (!placed)
            throw std::runtime_error("No free position near spawn for player");
    }

    players.emplace(id, std::move(player));
}

std::optional<Player> GameWorld::removePlayer(uint32_t id) {
    auto it = players.find(id);
    if (it == players.end()) return std::nullopt;
    Player player = std::move(it->second);
    players.erase(it);
    return player;
}

bool GameWorld::movePlayer(uint32_t id, Direction dir) {
    auto it = players.find(id);
    if (it == players.end()) return false;

    Player& p = it->second;
    if (p.isResurrecting()) return false;

    float step = p.getStepPx();

    float dx = 0.0f, dy = 0.0f;
    switch (dir) {
        case Direction::UP:    dy = -step; break;
        case Direction::DOWN:  dy =  step; break;
        case Direction::LEFT:  dx = -step; break;
        case Direction::RIGHT: dx =  step; break;
        default: return false;
    }

    float nx = p.getPixelX() + dx;
    float ny = p.getPixelY() + dy;

    // 1. Colision con el mapa
    if (!isMoveWalkable(nx, ny)) return false;

        std::cout << "[MOVE] dir=" << static_cast<int>(dir)
              << " from=(" << p.getPixelX() << "," << p.getPixelY() << ")"
              << " to=(" << nx << "," << ny << ")"
              << " tile=(" << collision.toTileX(nx) << "," << collision.toTileY(ny) << ")"
              << " walkable=" << collision.isWalkable(nx, ny)
              << std::endl;

    // 2. Colision con otras entidades (AABB)
    if (entityCollides(nx, ny, playerHitboxW, playerHitboxH, id)) return false;

    p.setPixelPos(nx, ny);
    return true;
}

Player& GameWorld::getPlayer(uint32_t id) {
    auto it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found: " + std::to_string(id));
    return it->second;
}

const Player& GameWorld::getPlayer(uint32_t id) const {
    return players.at(id);
}

bool GameWorld::canPlayerAct(uint32_t id) const {
    auto it = players.find(id);
    if (it == players.end()) return false;
    return it->second.isAlive();
}

// --- Posicion ---

int GameWorld::getTileX(uint32_t id) const {
    return players.at(id).getTileX();
}

int GameWorld::getTileY(uint32_t id) const {
    return players.at(id).getTileY();
}

float GameWorld::getPixelX(uint32_t id) const {
    return players.at(id).getPixelX();
}

float GameWorld::getPixelY(uint32_t id) const {
    return players.at(id).getPixelY();
}

// Para broadcast al cliente: usamos los pies directamente.
// El cliente ya sabe como posicionar el sprite con los pies en ese punto.
int GameWorld::getPixelXForBroadcast(uint32_t id) const {
    return static_cast<int>(players.at(id).getPixelX());
}

int GameWorld::getPixelYForBroadcast(uint32_t id) const {
    return static_cast<int>(players.at(id).getPixelY());
}

const Tile& GameWorld::getTileAt(int tileX, int tileY) const {
    return mapData.at(static_cast<uint16_t>(tileX),
                      static_cast<uint16_t>(tileY));
}

bool GameWorld::isPlayerAdjacentTo(uint32_t playerId,
                                    float targetPixelX,
                                    float targetPixelY) const {
    const Player& p = getPlayer(playerId);
    return collision.isAdjacent(p.getPixelX(), p.getPixelY(),
                                targetPixelX,  targetPixelY,
                                adjacencyThreshold);
}

// --- Experiencia y muerte ---

void GameWorld::giveExperience(uint32_t playerId, uint32_t exp,
                                float xpMultiplier) {
    Player& p = getPlayer(playerId);
    uint32_t limit    = formulas.calcExpLimit(p.getLevel());
    int16_t  newMaxHp = formulas.calcMaxHp(p.getRace(), p.getCls(), p.getLevel() + 1);
    int16_t  newMaxMana = formulas.calcMaxMana(p.getRace(), p.getCls(), p.getLevel() + 1);
    uint32_t finalExp = static_cast<uint32_t>(exp * xpMultiplier);
    p.addExperience(finalExp, limit, newMaxHp, newMaxMana);
}

GameWorld::DeathResult GameWorld::handlePlayerDeath(uint32_t targetId,
                                                     uint32_t attackerId) {
    Player& target = getPlayer(targetId);

    if (attackerId != 0) {
        Player& attacker = getPlayer(attackerId);
        uint32_t killExp = formulas.calcExpOnKill(target.getMaxHp(),
                                                   attacker.getLevel(),
                                                   target.getLevel());
        giveExperience(attackerId, killExp);
    }

    uint32_t safeGold  = formulas.calcMaxGold(target.getLevel());
    uint32_t excessGold = target.die(safeGold);
    std::vector<Item> items = target.purgeInventoryOnDeath();

    int tx = target.getTileX();
    int ty = target.getTileY();

    if (excessGold > 0)
        addGoldOnGround(excessGold, tx, ty);

    for (auto& item : items)
        addItemOnGround(std::move(item), tx, ty);

    return {excessGold, std::move(items)};
}

// --- Items en suelo ---

void GameWorld::addItemOnGround(Item item, int tileX, int tileY) {
    groundManager.addItem(std::move(item), tileX, tileY);
}

std::optional<Item> GameWorld::pickItemAt(int tileX, int tileY) {
    return groundManager.pickItemAt(tileX, tileY);
}

void GameWorld::addGoldOnGround(uint32_t amount, int tileX, int tileY) {
    groundManager.addGold(amount, tileX, tileY);
}

std::optional<uint32_t> GameWorld::pickGoldAt(int tileX, int tileY) {
    return groundManager.pickGoldAt(tileX, tileY);
}

// --- NPCs ---

void GameWorld::spawnNpc(const std::string& typeName, int tileX, int tileY) {
    // Convierte tile a pixeles (centro del tile)
    float px = static_cast<float>(tileX * tileSize + tileSize / 2);
    float py = static_cast<float>(tileY * tileSize + tileSize / 2);
    npcManager.spawnNpc(typeName, px, py);
}

const Npc& GameWorld::getNpc(uint32_t id) const {
    return npcManager.getNpcs().at(id);
}

const std::unordered_map<uint32_t, Npc>& GameWorld::getNpcs() const {
    return npcManager.getNpcs();
}

// --- Tick ---

GameWorld::WorldTickResult GameWorld::tick(float deltaSeconds) {
    WorldTickResult result;

    resurrectionSystem.tick(
        deltaSeconds * 1000.0f,
        [this](uint32_t pid, int tx, int ty) {
            Player& p = getPlayer(pid);
            p.stopResurrection();
            float px = static_cast<float>(tx * tileSize + tileSize / 2);
            float py = static_cast<float>(ty * tileSize + tileSize / 2);
            resurrectPlayer(pid, px, py);
        });

    tickPlayers(deltaSeconds, result);
    tickNpcs(result);
    return result;
}

void GameWorld::tickPlayers(float deltaSeconds, WorldTickResult& result) {
    for (auto& [id, player] : players) {
        if (!player.isAlive() && !player.isMeditating()) continue;

        float hpGained = formulas.calcHpRegen(player.getRace(), deltaSeconds);
        float manaGained = player.isMeditating()
            ? formulas.calcManaRegenMeditating(player.getCls(), player.getRace(), deltaSeconds)
            : formulas.calcManaRegen(player.getRace(), deltaSeconds);

        player.tick(hpGained, manaGained);
        result.playersChanged.push_back(id);
    }

    for (auto& [id, player] : players) {
        if (!player.isAlive()) continue;

        const Tile& tile = mapData.at(
            static_cast<uint16_t>(player.getTileX()),
            static_cast<uint16_t>(player.getTileY()));

        if (tile.type == TileType::DUNGEON_ENTRANCE ||
            tile.type == TileType::CAVERN_ENTRANCE) {
            if (!tile.targetMap.empty()) {
                result.instanceTransitions.push_back({
                    id, tile.targetMap,
                    player.getTileX(), player.getTileY()});
            }
        } else if (tile.type == TileType::EXIT) {
            result.instanceTransitions.push_back({
                id, "",
                player.getTileX(), player.getTileY()});
        }
    }
}

void GameWorld::tickNpcs(WorldTickResult& result) {
    auto npcResult = npcManager.tick(players);

    for (auto& intent : npcResult.moveIntents) {
        float nx = intent.toX;
        float ny = intent.toY ;

        // 1. Colision con el mapa
        if (!collision.isWalkable(nx, ny)) continue;
        if (!collision.isWalkable(nx, ny - npcHitboxH)) continue;

        // 2. Colision con jugadores y otros NPCs (AABB)
        if (entityCollides(nx, ny, npcHitboxW, npcHitboxH, intent.npcId)) continue;

        npcManager.applyMove(intent.npcId, nx, ny);
        result.npcsMoved.push_back(intent.npcId);
    }

    for (auto& attack : npcResult.attacks) {
        auto it = players.find(attack.targetPlayerId);
        if (it == players.end()) continue;

        it->second.takeDamage(attack.damage);
        result.playerHits.push_back({attack.targetPlayerId, attack.damage});

        if (!it->second.isAlive())
            handlePlayerDeath(attack.targetPlayerId, 0);
    }

    for (auto& death : npcResult.deaths) {
        if (death.goldDrop > 0)
            groundManager.addGold(death.goldDrop, death.tileX, death.tileY);

        if (!death.itemDrop.empty()) {
            try {
                groundManager.addItem(
                    itemRepo.createItem(death.itemDrop),
                    death.tileX, death.tileY);
            } catch (const std::exception& e) {
                std::cerr << "[GameWorld] item drop failed: " << e.what() << "\n";
            }
        }
        result.npcDeaths.push_back(death);
    }

    auto spawned = spawnManager.tick();
    result.npcSpawned = std::move(spawned);
}

// --- Resurreccion ---

void GameWorld::resurrectPlayer(uint32_t id, float pixelX, float pixelY) {
    Player& p = getPlayer(id);

    // Intenta la posicion pedida, si esta ocupada busca vecinos
    if (!collision.isWalkable(pixelX, pixelY) ||
        entityCollides(pixelX, pixelY, playerHitboxW, playerHitboxH, id))
    {
        bool placed = false;
        for (int dx = -1; dx <= 1 && !placed; dx++) {
            for (int dy = -1; dy <= 1 && !placed; dy++) {
                if (dx == 0 && dy == 0) continue;
                float nx = pixelX + dx * tileSize;
                float ny = pixelY + dy * tileSize;
                if (collision.isWalkable(nx, ny) &&
                    !entityCollides(nx, ny, playerHitboxW, playerHitboxH, id))
                {
                    p.resurrect(nx, ny);
                    placed = true;
                }
            }
        }
        if (!placed)
            p.resurrect(pixelX, pixelY);  // fuerza la posicion
    } else {
        p.resurrect(pixelX, pixelY);
    }
}

std::pair<float, float> GameWorld::findSafeSpawnNear(int tileX, int tileY) const {
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            int tx = tileX + dx;
            int ty = tileY + dy;
            if (!collision.isWalkableTile(tx, ty)) continue;

            const Tile& t = mapData.at(
                static_cast<uint16_t>(tx),
                static_cast<uint16_t>(ty));

            if (t.type != TileType::DUNGEON_ENTRANCE &&
                t.type != TileType::CAVERN_ENTRANCE &&
                t.type != TileType::EXIT) {
                float px = static_cast<float>(tx * tileSize + tileSize / 2);
                float py = static_cast<float>(ty * tileSize + tileSize / 2);
                return {px, py};
            }
        }
    }
    float px = static_cast<float>(tileX * tileSize + tileSize / 2);
    float py = static_cast<float>(tileY * tileSize + tileSize / 2);
    return {px, py};
}

// --- Ciudad ---

CityResult GameWorld::handleCityInteraction(uint32_t playerId,
                                             NpcType npcType,
                                             const CityCommand& cmd) {
    Player& player = getPlayer(playerId);
    return cityDispatcher.dispatch(npcType, cmd, player);
}

CityResult GameWorld::handleRemoteResurrect(uint32_t playerId) {
    Player& player = getPlayer(playerId);
    return priestHandler.handleRemoteResurrect(player);
}

std::optional<NpcType> GameWorld::getNpcTypeAtTile(int tileX, int tileY) const {
    if (!collision.isInBoundsTile(tileX, tileY)) return std::nullopt;
    NpcType t = mapData.at(
        static_cast<uint16_t>(tileX),
        static_cast<uint16_t>(tileY)).npc;
    return t != NpcType::NONE ? std::optional<NpcType>(t) : std::nullopt;
}


bool GameWorld::isMoveWalkable(float px, float py) const {
    // Pies
    if (!collision.isWalkable(px, py)) return false;
    // Punto medio del cuerpo
    if (!collision.isWalkable(px, py - playerHitboxH / 2.0f)) return false;
    // Cabeza (top del hitbox)
    if (!collision.isWalkable(px, py - playerHitboxH)) return false;
    // Costados
    if (!collision.isWalkable(px - playerHitboxW / 2.0f, py - playerHitboxH / 2.0f)) return false;
    if (!collision.isWalkable(px + playerHitboxW / 2.0f, py - playerHitboxH / 2.0f)) return false;
    return true;
}