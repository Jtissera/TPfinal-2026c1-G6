#include "playerRepository.h"

void PlayerRepository::save(uint32_t clientId, Player player)
{
  players.emplace(clientId, std::move(player));
}

Player *PlayerRepository::get(uint32_t clientId)
{
  std::unordered_map<uint32_t, Player>::iterator it = players.find(clientId);
  if (it == players.end())
  {
    return nullptr;
  }
  return &it->second;
}

void PlayerRepository::remove(uint32_t clientId)
{
  players.erase(clientId);
}