#include "gameWorld.h"
#include <iostream>

GameWorld::GameWorld(const std::string& mapPath)
    : mapData(MapSerializer::load(mapPath)) {}

void GameWorld::addPlayer(Player player) {
    uint32_t id = player.getId();
    std::cout << "[GameWorld] addPlayer id=" << id 
              << " x=" << player.getX() << " y=" << player.getY() << std::endl;
    players.emplace(id, std::move(player));
}
void GameWorld::removePlayer(uint32_t id) {
    players.erase(id);
}

bool GameWorld::movePlayer(uint32_t id, Direction dir) {
    auto it = players.find(id);
    if (it == players.end()) {
        std::cout << "[GameWorld] movePlayer: player " << id << " not found" << std::endl;
        return false;
    }

    Player& p = it->second;
    int nx = p.getX(), ny = p.getY();

    switch (dir) {
        case Direction::UP:    ny -= SPEED; break;
        case Direction::DOWN:  ny += SPEED; break;
        case Direction::LEFT:  nx -= SPEED; break;
        case Direction::RIGHT: nx += SPEED; break;
        default: return false;
    }

    if (nx + HITBOX_OFFSET_X + HITBOX_W > 20 * 96) return false;
    if (ny + HITBOX_OFFSET_Y + HITBOX_H > 15 * 96) return false;
    if (wouldCollide(nx, ny)) return false;

    p.setPos(nx, ny);
    return true;
}

bool GameWorld::wouldCollide(int x, int y) const {
    static constexpr int TILE_SIZE = 96;
    int hx = x + HITBOX_OFFSET_X;
    int hy = y + HITBOX_OFFSET_Y;
    int hw = HITBOX_W - 1;
    int hh = HITBOX_H - 1;

    auto blocked = [&](int px, int py) -> bool {
        if (px < 0 || py < 0) return true;
        int tx = px / TILE_SIZE;
        int ty = py / TILE_SIZE;
        if (tx >= mapData.width() || ty >= mapData.height()) return true;
        return !mapData.at(tx, ty).walkable;
    };

    return blocked(hx,      hy     ) ||
           blocked(hx + hw, hy     ) ||
           blocked(hx,      hy + hh) ||
           blocked(hx + hw, hy + hh);
}

int GameWorld::getX(uint32_t id) const { return players.at(id).getX(); }
int GameWorld::getY(uint32_t id) const { return players.at(id).getY(); }

const Player& GameWorld::getPlayer(uint32_t id) const {
    auto it = players.find(id);
    if (it == players.end())
        throw std::runtime_error("Player not found");

    return it->second;
}
std::vector<uint32_t> GameWorld::tick(float deltaSeconds) {
    std::vector<uint32_t> changed;

    for (auto& [id, player] : players) {
        if (!player.isAlive() && !player.isMeditating())
            continue;

        float hpGained = formulas.calcHpRegen(
            player.getRace(),
            deltaSeconds
        );

        float manaGained;
        if (player.isMeditating()) {
            manaGained = formulas.calcManaRegenMeditating(
                player.getCls(),
                player.getRace(),
                deltaSeconds
            );
        } else {
            manaGained = formulas.calcManaRegen(
                player.getRace(),
                deltaSeconds
            );
        }

        player.tick(hpGained, manaGained);
        changed.push_back(id);
    }

    return changed;
}