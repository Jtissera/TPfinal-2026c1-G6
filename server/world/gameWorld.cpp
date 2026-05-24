#include "gameWorld.h"
#include <iostream>

GameWorld::GameWorld(const std::string& mapPath)
    : mapData(MapSerializer::load(mapPath)) {}

void GameWorld::addPlayer(uint32_t id, int x, int y) {
    players[id] = {id, x, y};
}

void GameWorld::removePlayer(uint32_t id) {
    players.erase(id);
}

bool GameWorld::movePlayer(uint32_t id, Direction dir) {
    auto it = players.find(id);
    if (it == players.end()) return false;

    Player& p = it->second;
    int nx = p.x, ny = p.y;

    switch (dir) {
        case Direction::UP:    ny -= Player::SPEED; break;
        case Direction::DOWN:  ny += Player::SPEED; break;
        case Direction::LEFT:  nx -= Player::SPEED; break;
        case Direction::RIGHT: nx += Player::SPEED; break;
        default: return false;
    }


    if (nx + Player::HITBOX_OFFSET_X + Player::HITBOX_W > 20 * 96) return false;
    if (ny + Player::HITBOX_OFFSET_Y + Player::HITBOX_H > 15 * 96) return false;

    if (wouldCollide(nx, ny)) return false;

    p.x = nx;
    p.y = ny;
    return true;
}

bool GameWorld::wouldCollide(int x, int y) const {
    static constexpr int TILE_SIZE = 96;

    int hx = x + Player::HITBOX_OFFSET_X;
    int hy = y + Player::HITBOX_OFFSET_Y;
    int hw = Player::HITBOX_W - 1;
    int hh = Player::HITBOX_H - 1;

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

int GameWorld::getX(uint32_t id) const { return players.at(id).x; }
int GameWorld::getY(uint32_t id) const { return players.at(id).y; }