
#pragma once

#include <map>
#include <mutex>
#include <optional>

#include "../../common/game/direction.h"
#include "../../common/game/position.h"
#include "../../editor/map/mapData.h"

class GameState {
public:
  explicit GameState(MapData &map);

  void addPlayer(uint32_t clientId, Position spawnPos);
  void removePlayer(uint32_t clientId);

  std::optional<Position> tryMove(uint32_t clientId, Direction dir);
  Position getPosition(uint32_t clientId) const;

private:
  Position calcDestination(Position current, Direction dir) const;

  MapData &map;
  std::map<uint32_t, Position> players;
};