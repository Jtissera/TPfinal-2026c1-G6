#include "gameWorld.h"
#include <iostream>
#include <stdexcept>

GameWorld::GameWorld(const std::string& mapPath)
    : mapData(MapSerializer::load(mapPath))
    , collision(mapData)
{}

GameWorld::GameWorld(MapData mapData)
    : mapData(std::move(mapData))
    , collision(this->mapData)  
{}

void GameWorld::addPlayer(Player player) {
    uint32_t id = player.getId();
    players.emplace(id, std::move(player));
}

void GameWorld::removePlayer(uint32_t id) {
    players.erase(id);
}

bool GameWorld::movePlayer(uint32_t id, Direction dir) {
    auto it = players.find(id);
    if (it == players.end()) return false;

    Player& p = it->second;

    
    int nx = p.getX();
    int ny = p.getY();

    switch (dir) {
        case Direction::UP:    ny -= SPEED; break;
        case Direction::DOWN:  ny += SPEED; break;
        case Direction::LEFT:  nx -= SPEED; break;
        case Direction::RIGHT: nx += SPEED; break;
        default: return false;
    }

    const Hitbox& hb = p.getHitbox();

    if (!collision.isInBounds(nx, ny, hb)) return false;
    if (collision.wouldCollide(nx, ny, hb)) return false;

    p.setPos(nx, ny);
    return true;
}

std::vector<uint32_t> GameWorld::tick(float deltaSeconds) {
    std::vector<uint32_t> changed;
    for (auto& [id, player] : players) {
        if (!player.isAlive() && !player.isMeditating()) continue;

        float hpGained   = formulas.calcHpRegen(player.getRace(), deltaSeconds);
        float manaGained = player.isMeditating()
            ? formulas.calcManaRegenMeditating(player.getCls(), player.getRace(), deltaSeconds)
            : formulas.calcManaRegen(player.getRace(), deltaSeconds);

        player.tick(hpGained, manaGained);
        changed.push_back(id);
    }
    return changed;
}

int GameWorld::getX(uint32_t id) const { return players.at(id).getX(); }
int GameWorld::getY(uint32_t id) const { return players.at(id).getY(); }

Player& GameWorld::getPlayer(uint32_t id) {
    auto it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found");
    return it->second;
}

void GameWorld::addItemOnGround(Item item, int x, int y) {
    groundItems.push_back({std::move(item), x, y});
}

std::optional<Item> GameWorld::pickItemAt(int x, int y) {
    static constexpr int PICK_RADIUS = 96; //un tile, hay que moverlo a TOML
    for (auto it = groundItems.begin(); it != groundItems.end(); ++it) {
        if (std::abs(it->x - x) <= PICK_RADIUS &&
            std::abs(it->y - y) <= PICK_RADIUS) {
            Item found = std::move(it->item);
            groundItems.erase(it);
            return found;
        }
    }
    return std::nullopt;
}

//es igual, hay que ver que hacer
std::optional<uint32_t> GameWorld::pickGoldAt(int x, int y) {
    static constexpr int PICK_RADIUS = 96;
    for (auto it = groundGold.begin(); it != groundGold.end(); ++it) {
        if (std::abs(it->x - x) <= PICK_RADIUS &&
            std::abs(it->y - y) <= PICK_RADIUS) {
            uint32_t amount = it->amount;
            groundGold.erase(it);
            return amount;
        }
    }
    return std::nullopt;
}

GameWorld::DeathResult GameWorld::handlePlayerDeath(uint32_t targetId, uint32_t attackerId) {
    Player& target   = getPlayer(targetId);
    Player& attacker = getPlayer(attackerId);

    uint32_t killExp = formulas.calcExpOnKill(target.getMaxHp(), attacker.getLevel(), target.getLevel());
    attacker.addExperience(killExp);

    uint32_t excessGold = target.die();
    std::vector<Item> items = target.purgeInventoryOnDeath();

    if (excessGold > 0)
        groundGold.push_back({excessGold, target.getX(), target.getY()});

    for (auto& item : items)
        addItemOnGround(std::move(item), target.getX(), target.getY());

    return {excessGold, std::move(items)};
}