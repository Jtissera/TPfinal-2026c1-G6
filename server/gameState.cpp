#include "gameState.h"
#include <stdexcept>

GameState::GameState(MapData &map) : map(map) {}

void GameState::addPlayer(uint32_t clientId, Position spawnPos) {
  players[clientId] = spawnPos;
}

void GameState::removePlayer(uint32_t clientId) { players.erase(clientId); }

Position GameState::calcDestination(Position current, Direction dir) const {
  Position dest = current;
  switch (dir) {
  case Direction::UP:
    dest.y -= 1;
    break;
  case Direction::DOWN:
    dest.y += 1;
    break;
  case Direction::LEFT:
    dest.x -= 1;
    break;
  case Direction::RIGHT:
    dest.x += 1;
    break;
  default:
    break;
  }
  return dest;
}

std::optional<Position> GameState::tryMove(uint32_t clientId, Direction dir) {
  auto it = players.find(clientId);
  if (it == players.end())
    return std::nullopt;

  Position dest = calcDestination(it->second, dir);

  if (!map.inBounds(dest.x, dest.y))
    return std::nullopt;

  if (!map.at(dest.x, dest.y).walkable)
    return std::nullopt;

  it->second = dest;
  return dest;
}

Position GameState::getPosition(uint32_t clientId) const {
  auto it = players.find(clientId);
  if (it == players.end())
    throw std::runtime_error("Player not found");
  return it->second;
}