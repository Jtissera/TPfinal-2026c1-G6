#pragma once

#include "BinaryArchiveFile.h"

#include <cstdint>
#include <string>
#include <vector>

struct GameRecord
{
    uint32_t gameId    = 0;
    uint8_t maxPlayers = 0;
    uint8_t _pad[3]    = {};
    char gameName[64]  = {};
    char mapPath[128]  = {};
};
static_assert(sizeof(GameRecord) == 200, "GameRecord size mismatch");

class GameArchive
{
public:
    GameArchive(const std::string &datPath, const std::string &indexPath);

    void save(uint32_t gameId, const std::string &gameName,
              const std::string &mapPath, uint8_t maxPlayers);

    std::vector<GameRecord> loadAll() const;
    uint32_t maxGameId() const;

private:
    BinaryArchiveFile<uint32_t> archive_;
};