#include "PlayerManager.h"

PlayerManager::PlayerManager(OccupancySystem &occupancy,
                             const CollisionSystem &collision,
                             const GameFormulas &formulas,
                             ItemRepository &itemRepo,
                             ResurrectionSystem &resurrectionSystem,
                             ClanManager &clanManager,
                             const toml::table &config)
    : occupancy(occupancy),
      collision(collision),
      formulas(formulas),
      itemRepo(itemRepo),
      resurrectionSystem(resurrectionSystem),
      clanManager(clanManager),
      players(),
      tileSize(config["world"]["tile_size"].value_or(96)),
      playerMoveStep(config["player"]["move_step"].value_or(8.0f))
{
}

std::optional<std::pair<int, int>> PlayerManager::findAndOccupyAdjacentTile(
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
                return std::make_pair(tx, ty);
        }
    }
    return std::nullopt;
}

void PlayerManager::addPlayer(Player player)
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
        throw std::runtime_error("No free tile near spawn for player");

    player.setTilePos(freeTile->first, freeTile->second);
    players.emplace(id, std::move(player));
}

std::optional<Player> PlayerManager::removePlayer(uint32_t id)
{
    std::unordered_map<uint32_t, Player>::iterator it = players.find(id);
    if (it == players.end())
        return std::nullopt;

    occupancy.free(it->second.getTileX(), it->second.getTileY());
    Player player = std::move(it->second);
    players.erase(it);
    return player;
}

bool PlayerManager::movePlayer(uint32_t id, Direction dir)
{
    std::unordered_map<uint32_t, Player>::iterator it = players.find(id);
    if (it == players.end())
        return false;

    Player &p = it->second;
    if (p.isResurrecting())
        return false;

    float dx = 0.0f;
    float dy = 0.0f;

    switch (dir)
    {
    case Direction::UP:    dy = -playerMoveStep; break;
    case Direction::DOWN:  dy =  playerMoveStep; break;
    case Direction::LEFT:  dx = -playerMoveStep; break;
    case Direction::RIGHT: dx =  playerMoveStep; break;
    default: return false;
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
        return false;

    if ((oldTileX != newTileX || oldTileY != newTileY) &&
        !occupancy.move(oldTileX, oldTileY, newTileX, newTileY, id))
        return false;

    p.setPixelPos(nextX, nextY);
    return true;
}

Player &PlayerManager::getPlayer(uint32_t id)
{
    std::unordered_map<uint32_t, Player>::iterator it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found: " + std::to_string(id));
    return it->second;
}

const Player &PlayerManager::getPlayer(uint32_t id) const
{
    std::unordered_map<uint32_t, Player>::const_iterator it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found: " + std::to_string(id));
    return it->second;
}

bool PlayerManager::hasPlayer(uint32_t id) const
{
    return players.find(id) != players.end();
}

bool PlayerManager::canPlayerAct(uint32_t id) const
{
    std::unordered_map<uint32_t, Player>::const_iterator it = players.find(id);
    if (it == players.end())
        return false;
    return it->second.isAlive();
}

int PlayerManager::getTileX(uint32_t id) const  { return players.at(id).getTileX(); }
int PlayerManager::getTileY(uint32_t id) const  { return players.at(id).getTileY(); }
int PlayerManager::getPixelX(uint32_t id) const { return static_cast<int>(players.at(id).getPixelX()); }
int PlayerManager::getPixelY(uint32_t id) const { return static_cast<int>(players.at(id).getPixelY()); }

void PlayerManager::giveExperience(uint32_t playerId, uint32_t exp, float xpMultiplier)
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

DeathResult PlayerManager::handlePlayerDeath(uint32_t targetId,
                                                            uint32_t attackerId,
                                                            GroundManager &groundManager)
{
    Player &target = getPlayer(targetId);

    if (target.isGhost())
        return {0, 0, {}, 0, 0};

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
        goldInstanceId = groundManager.addGold(excessGold, tileX, tileY);

    std::vector<Item> items = target.purgeInventoryOnDeath();
    for (const Item &item : items)
        groundManager.addItem(item, tileX, tileY);

    occupancy.free(tileX, tileY);

    DeathResult result;
    result.excessGold = excessGold;
    result.goldInstanceId = goldInstanceId;
    result.droppedItems = std::move(items);
    result.tileX = tileX;
    result.tileY = tileY;
    return result;
}

void PlayerManager::resurrectPlayer(uint32_t id, int spawnTileX, int spawnTileY)
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
        p.resurrect(freeTile->first, freeTile->second);
}

void PlayerManager::handleResurrectionComplete(uint32_t playerId, int tileX, int tileY,
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

void PlayerManager::tickPlayers(float deltaSeconds, const MapData &mapData,
                                WorldTickResult &result)
{
    tickPlayerStats(deltaSeconds, result);
    checkPlayerTileEvents(mapData, result);
}

void PlayerManager::tickPlayerStats(float deltaSeconds, WorldTickResult &result)
{
    for (std::pair<const uint32_t, Player> &entry : players)
    {
        Player &player = entry.second;
        if (!player.isAlive() && !player.isMeditating())
            continue;

        const float hpGained = formulas.calcHpRegen(player.getRace(), deltaSeconds);
        const float manaGained = player.isMeditating()
            ? formulas.calcManaRegenMeditating(player.getCls(), player.getRace(), deltaSeconds)
            : formulas.calcManaRegen(player.getRace(), deltaSeconds);

        if (player.tick(hpGained, manaGained))
            result.playersChanged.push_back(entry.first);
    }
}

void PlayerManager::checkPlayerTileEvents(const MapData &mapData, WorldTickResult &result)
{
    for (std::pair<const uint32_t, Player> &entry : players)
    {
        const Player &player = entry.second;
        if (player.isMeditating())
            continue;

        const Tile &tile = mapData.at(
            static_cast<uint16_t>(player.getTileX()),
            static_cast<uint16_t>(player.getTileY()));

        if (tile.type == TileType::DUNGEON_ENTRANCE ||
            tile.type == TileType::CAVERN_ENTRANCE)
        {
            if (!tile.targetMap.empty())
                result.instanceTransitions.push_back(
                    {entry.first, tile.targetMap, player.getTileX(), player.getTileY()});
        }
        else if (tile.type == TileType::EXIT)
        {
            result.instanceTransitions.push_back(
                {entry.first, "", player.getTileX(), player.getTileY()});
        }
    }
}

std::optional<uint32_t> PlayerManager::findPlayerIdByName(const std::string &name) const
{
    for (const std::pair<const uint32_t, Player> &entry : players)
    {
        if (entry.second.getName() == name)
            return entry.first;
    }
    return std::nullopt;
}

int PlayerManager::countClanAlliesNear(const Player &player, int radiusTiles) const
{
    const std::optional<std::pair<std::string, bool>> playerClanInfo =
        clanManager.findClanInfoForMember(player.getName());
    if (!playerClanInfo)
        return 0;

    const std::string &playerClan = playerClanInfo->first;
    int count = 0;

    for (const std::pair<const uint32_t, Player> &entry : players)
    {
        if (entry.first == player.getId() || !entry.second.isAlive())
            continue;

        const std::optional<std::pair<std::string, bool>> otherClanInfo =
            clanManager.findClanInfoForMember(entry.second.getName());

        if (otherClanInfo && otherClanInfo->first == playerClan)
        {
            const int dx = std::abs(entry.second.getTileX() - player.getTileX());
            const int dy = std::abs(entry.second.getTileY() - player.getTileY());
            if (dx <= radiusTiles && dy <= radiusTiles)
                ++count;
        }
    }
    return count;
}

std::vector<uint32_t> PlayerManager::getOnlineClanMemberIds(const std::string &clanName) const
{
    std::vector<uint32_t> members;

    for (const std::pair<const uint32_t, Player> &entry : players)
    {
        if (!entry.second.isAlive())
            continue;

        const std::optional<std::pair<std::string, bool>> clanInfo =
            clanManager.findClanInfoForMember(entry.second.getName());

        if (clanInfo && clanInfo->first == clanName)
            members.push_back(entry.first);
    }
    return members;
}

const std::unordered_map<uint32_t, Player> &PlayerManager::getPlayers() const
{
    return players;
}

void PlayerManager::loadInitialInventoryForPlayer(Player &player)
{
    if (player.hasReceivedInitialInventory())
        return;

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