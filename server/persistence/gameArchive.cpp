#include "gameArchive.h"

#include <cstring>
#include <iostream>

GameArchive::GameArchive(const std::string &datPath,
                         const std::string &indexPath)
    : archive_(datPath, indexPath, sizeof(GameRecord), sizeof(uint32_t))
{
}

void GameArchive::save(uint32_t gameId, const std::string &gameName,
                       const std::string &mapPath, uint8_t maxPlayers)
{
    if (archive_.hasKey(gameId))
        return;

    GameRecord rec;
    std::memset(&rec, 0, sizeof(rec));
    rec.gameId     = gameId;
    rec.maxPlayers = maxPlayers;
    std::strncpy(rec.gameName, gameName.c_str(), sizeof(rec.gameName) - 1);
    std::strncpy(rec.mapPath, mapPath.c_str(), sizeof(rec.mapPath) - 1);

    const uint64_t offset = archive_.allocateSlot(gameId);
    archive_.writeRecord(offset, &rec, sizeof(rec));
    archive_.appendToIndex(gameId, offset);

    std::cout << "[GameArchive] guardada gameId=" << gameId
              << " name='" << gameName << "'" << std::endl;
}

std::vector<GameRecord> GameArchive::loadAll() const
{
    std::vector<GameRecord> result;
    const std::vector<uint64_t> offsets = archive_.allOffsets();

    for (uint64_t offset : offsets)
    {
        GameRecord rec;
        if (archive_.readRecord(offset, &rec, sizeof(rec)))
            result.push_back(rec);
    }

    return result;
}

uint32_t GameArchive::maxGameId() const
{
    const std::vector<uint64_t> offsets = archive_.allOffsets();
    uint32_t max = 0;

    for (uint64_t offset : offsets)
    {
        GameRecord rec;
        if (archive_.readRecord(offset, &rec, sizeof(rec)))
        {
            if (rec.gameId > max)
                max = rec.gameId;
        }
    }

    return max;
}