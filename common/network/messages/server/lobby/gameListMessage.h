#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include <cstdint>
#include <string>
#include <vector>

struct GameInfo
{
    uint32_t gameId;
    std::string gameName;
    uint8_t playerCount;
    uint8_t maxPlayers;
    std::string mapPath;
};

class GameListMessage : public Message
{
public:
    explicit GameListMessage(std::vector<GameInfo> games);

    const std::vector<GameInfo> &getGames() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::vector<GameInfo> games;
};