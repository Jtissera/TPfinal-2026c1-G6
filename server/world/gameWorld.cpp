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

const Player& GameWorld::getPlayer(uint32_t id) const {
    auto it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found");
    return it->second;
}