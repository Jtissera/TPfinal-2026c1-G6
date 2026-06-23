#pragma once
#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include <cstdint>
#include <string>

class CreateGameMessage : public Message
{
public:
    CreateGameMessage(std::string gameName, uint8_t maxPlayers, std::string mapPath = "");

    const std::string &getGameName() const;
    uint8_t getMaxPlayers() const;
    const std::string &getMapPath() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::string gameName;
    uint8_t maxPlayers;
    std::string mapPath;
};