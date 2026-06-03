#include "gameWorld.h"

//Esta hace cada vez mas, habria que pensar que hacer si sigue creciendo

GameWorld::GameWorld(const std::string& mapPath,
                     NpcFactory& npcFactory,
                     ItemRepository& itemRepo)
    : mapData(MapSerializer::load(mapPath))
    , collision(mapData)
    , npcManager(npcFactory, collision)
    , itemRepo(itemRepo)
{
    spawnMapNpcs();
}

GameWorld::GameWorld(MapData mapData,
                     NpcFactory& npcFactory,
                     ItemRepository& itemRepo)
    : mapData(std::move(mapData))
    , collision(this->mapData)
    , npcManager(npcFactory, collision)
    , itemRepo(itemRepo)
{
    spawnMapNpcs();
}


//busca tile para spawnear y sino adyacentes

void GameWorld::addPlayer(Player player) {
    loadInitialInventoryForPlayer(player);
    const uint32_t id = player.getId();
    int tx = player.getTileX();
    int ty = player.getTileY();

    if (!occupancy.occupy(tx, ty, id)) {
       
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                if (dx == 0 && dy == 0) continue;
                if (collision.isWalkable(tx + dx, ty + dy) && occupancy.occupy(tx + dx, ty + dy, id)) {
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

void GameWorld::removePlayer(uint32_t id) {
    auto it = players.find(id);
    if (it == players.end()) return;
    occupancy.free(it->second.getTileX(), it->second.getTileY());
    players.erase(it);
}
bool GameWorld::movePlayer(uint32_t id, Direction dir) {
    // Buscamos al jugador por id.
    auto it = players.find(id);

    // Si no existe, no podemos moverlo.
    if (it == players.end()) {
        return false;
    }

    // Obtenemos referencia al jugador.
    Player& p = it->second;

    // valor fijo por ahora, para no romper cosas. PASAR AL TOML PORQUE ES LA VELOCIDAD DEL JGUADOR EN QUE SE MUEVE
    constexpr float PLAYER_MOVE_STEP = 8.0f;

    // Calculamos el desplazamiento deseado.
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

    // Posición actual en píxeles.
    float currentX = p.getPixelX();
    float currentY = p.getPixelY();

    // Posición tentativa en píxeles.
    float nextX = currentX + dx;
    float nextY = currentY + dy;

    // Tile actual antes de moverse.
    int oldTileX = p.getTileX();
    int oldTileY = p.getTileY();

    // Tile al que caería después de moverse.
    int newTileX = static_cast<int>(nextX) / TILE_SIZE;
    int newTileY = static_cast<int>(nextY) / TILE_SIZE;

    // Si el nuevo tile no es caminable, bloqueamos el movimiento.
    if (!collision.isWalkable(newTileX, newTileY)) {
        return false;
    }

    // Si cambió de tile, actualizamos la ocupación.
    // Si sigue dentro del mismo tile, no hace falta tocar occupancy.
    if (oldTileX != newTileX || oldTileY != newTileY) {
        if (!occupancy.move(oldTileX, oldTileY, newTileX, newTileY, id)) {
            return false;
        }
    }

    // Aplicamos el movimiento real en píxeles.
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
int GameWorld::getPixelX(uint32_t id) const {
    // Devuelve la posición real en píxeles.
    // No multiplicamos por TILE_SIZE porque eso vuelve a generar saltos de 96px.
    return static_cast<int>(players.at(id).getPixelX());
}

int GameWorld::getPixelY(uint32_t id) const {
    // Devuelve la posición real en píxeles.
    // No multiplicamos por TILE_SIZE porque eso vuelve a generar saltos de 96px.
    return static_cast<int>(players.at(id).getPixelY());
}


void GameWorld::giveExperience(uint32_t playerId, uint32_t exp) {
    Player& p = getPlayer(playerId);

    uint32_t limit      = formulas.calcExpLimit(p.getLevel());
    int16_t  newMaxHp   = formulas.calcMaxHp(p.getRace(), p.getCls(), p.getLevel() + 1);
    int16_t  newMaxMana = formulas.calcMaxMana(p.getRace(), p.getCls(), p.getLevel() + 1);

    p.addExperience(exp, limit, newMaxHp, newMaxMana);
}


GameWorld::DeathResult GameWorld::handlePlayerDeath(uint32_t targetId,
                                                     uint32_t attackerId) {
    Player& target = getPlayer(targetId);

    // Exp al atacante si existe (0 = mató un NPC o muerte por otra causa)
    if (attackerId != 0) {
        Player& attacker = getPlayer(attackerId);
        uint32_t killExp = formulas.calcExpOnKill(
            target.getMaxHp(), attacker.getLevel(), target.getLevel());
        giveExperience(attackerId, killExp);
    }

    uint32_t safeGold   = formulas.calcMaxGold(target.getLevel());
    uint32_t excessGold = target.die(safeGold);

    std::vector<Item> items = target.purgeInventoryOnDeath();

    if (excessGold > 0)
        addGoldOnGround(excessGold, target.getTileX(), target.getTileY());

    for (auto& item : items)
        addItemOnGround(std::move(item), target.getTileX(), target.getTileY());

    // Liberar el tile
    occupancy.free(target.getTileX(), target.getTileY());

    return {excessGold, std::move(items)};
}



void GameWorld::addItemOnGround(Item item, int tileX, int tileY) {
    groundItems.push_back({std::move(item), tileX, tileY});
}

std::optional<Item> GameWorld::pickItemAt(int tileX, int tileY) {
    for (auto it = groundItems.begin(); it != groundItems.end(); ++it) {
        if (it->tileX == tileX && it->tileY == tileY) {
            Item found = std::move(it->item);
            groundItems.erase(it);
            return found;
        }
    }
    return std::nullopt;
}

void GameWorld::addGoldOnGround(uint32_t amount, int tileX, int tileY) {
    groundGold.push_back({amount, tileX, tileY});
}

std::optional<uint32_t> GameWorld::pickGoldAt(int tileX, int tileY) {
    for (auto it = groundGold.begin(); it != groundGold.end(); ++it) {
        if (it->tileX == tileX && it->tileY == tileY) {
            uint32_t amount = it->amount;
            groundGold.erase(it);
            return amount;
        }
    }
    return std::nullopt;
}

//switch feo
void GameWorld::spawnNpc(const std::string& typeName, int tileX, int tileY) {
    if (!collision.isWalkable(tileX, tileY)) return;
    if (occupancy.isOccupied(tileX, tileY)) return;
    uint32_t npcId = npcManager.spawnNpc(typeName, tileX, tileY);
    occupancy.occupy(tileX, tileY, npcId); 
}

void GameWorld::spawnMapNpcs() {
    for (uint16_t y = 0; y < mapData.height(); y++) {
        for (uint16_t x = 0; x < mapData.width(); x++) {
            const Tile& tile = mapData.at(x, y);
            if (tile.npc == NpcType::NONE) continue;

            std::string typeName;
            switch (tile.npc) {
                case NpcType::GOBLIN:   typeName = "goblin";   break;
                case NpcType::SKELETON: typeName = "skeleton"; break;
                case NpcType::ZOMBIE:   typeName = "zombie";   break;
                default: continue;
            }

                        spawnPoints.push_back({typeName, {x, y}});
            
            // Spawn inicial con desplazamiento aleatorio también
            int attempts = 0;
            while (attempts < 10) {
                int dx = (std::rand() % 7) - 3;
                int dy = (std::rand() % 7) - 3;
                int tx = x + dx;
                int ty = y + dy;
                if (collision.isWalkable(tx, ty) && !occupancy.isOccupied(tx, ty)) {
                    spawnNpc(typeName, tx, ty);
                    break;
                }
                attempts++;
            }
        }
    }
}

const std::unordered_map<uint32_t, Npc>& GameWorld::getNpcs() const {
    return npcManager.getNpcs();
}

//metodo grande, se puede seperar
GameWorld::WorldTickResult GameWorld::tick(float deltaSeconds) {
    WorldTickResult result;

    for (auto& [id, player] : players) {
        if (!player.isAlive() && !player.isMeditating()) continue;

        float hpGained   = formulas.calcHpRegen(player.getRace(), deltaSeconds);
        float manaGained = player.isMeditating()
            ? formulas.calcManaRegenMeditating(
                player.getCls(), player.getRace(), deltaSeconds)
            : formulas.calcManaRegen(player.getRace(), deltaSeconds);

        player.tick(hpGained, manaGained);
        result.playersChanged.push_back(id);
    }

    auto npcResult = npcManager.tick(players);

    // actualizar occupancy
    for (auto& intent : npcResult.moveIntents) {

        if (!collision.isWalkable(intent.toX, intent.toY)) continue;

        if (!occupancy.move(intent.fromX, intent.fromY, 
                            intent.toX,   intent.toY, 
                            intent.npcId)) continue;

        npcManager.applyMove(intent.npcId, intent.toX, intent.toY);

        result.npcsMoved.push_back(intent.npcId);
    }

    for (auto& attack : npcResult.attacks) {
        auto it = players.find(attack.targetPlayerId);
        if (it == players.end()) continue;

        it->second.takeDamage(attack.damage);
        result.playerHits.push_back({attack.targetPlayerId, attack.damage});

        if (!it->second.isAlive()) {
            handlePlayerDeath(attack.targetPlayerId, 0);
        }
    }

    for (auto& death : npcResult.deaths) {
        occupancy.free(death.tileX, death.tileY);

        if (death.goldDrop > 0)
            addGoldOnGround(death.goldDrop, death.tileX, death.tileY);

        if (!death.itemDrop.empty()) {
            try {
                Item item = itemRepo.createItem(death.itemDrop);
                addItemOnGround(std::move(item), death.tileX, death.tileY);
            } catch (...) {}
        }

        result.npcDeaths.push_back(death);
    }

    spawnTickCounter++;
    if (spawnTickCounter >= SPAWN_EVERY_N_TICKS) {
        spawnTickCounter = 0;
    
    // Spawnear un lote hasta llegar al límite
        int toSpawn = std::min(SPAWN_BATCH_SIZE, MAX_NPCS - npcManager.count());
    
        for (int i = 0; i < toSpawn && !spawnPoints.empty(); i++) {
            auto& [type, pos] = spawnPoints[std::rand() % spawnPoints.size()];
        
        // Desplazamiento aleatorio para que no campeen
            int attempts = 0;
            while (attempts < 10) {
                int dx = (std::rand() % 7) - 3;  // lo muevo en 3
                int dy = (std::rand() % 7) - 3;
                int tx = pos.first  + dx;
                int ty = pos.second + dy;
            
                if (collision.isWalkable(tx, ty) && !occupancy.isOccupied(tx, ty)) {
                    spawnNpc(type, tx, ty);
                    break;
                }
                attempts++;
            }
        }
    }

    return result;
}

void GameWorld::resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY) {
    Player& p = getPlayer(id);
    occupancy.free(p.getTileX(), p.getTileY());

    // Buscar tile libre cerca del spawn
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

bool GameWorld::hasNpc(uint32_t npcId) const {
    return npcManager.hasNpc(npcId);
}
bool GameWorld::hasPlayer(uint32_t playerId) const {
    return players.find(playerId) != players.end();
}

bool GameWorld::damageNpc(uint32_t npcId,int16_t damage,uint32_t attackerPlayerId) {
    return npcManager.damageNpc(npcId,damage,attackerPlayerId);
}

Npc& GameWorld::getNpc(uint32_t npcId) {
    return npcManager.getNpc(npcId);
}

const Npc& GameWorld::getNpc(uint32_t npcId) const {
    return npcManager.getNpc(npcId);
}

const std::unordered_map<uint32_t, Player>& GameWorld::getPlayers() const {
    return players;
}

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
        return;
    }

    if (className == "Warrior") {
        player.getInventory().addItem(itemRepo.createItem("espada"));
        player.getInventory().addItem(itemRepo.createItem("armadura_placas"));
        player.getInventory().addItem(itemRepo.createItem("escudo_tortuga"));
        player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
        return;
    }

    std::cerr << "[GameWorld][Inventory] clase desconocida='"
              << className
              << "', cargando inventario default."
              << std::endl;

    player.getInventory().addItem(itemRepo.createItem("espada"));
    player.getInventory().addItem(itemRepo.createItem("pocion_vida"));
}