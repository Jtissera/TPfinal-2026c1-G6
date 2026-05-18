#pragma once

#include <string>

#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"

class CreateGameMessage : public Message
{
public:
    explicit CreateGameMessage(std::string gameName, uint8_t maxPlayers);

    const std::string &getGameName() const;
    uint8_t getMaxPlayers() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::string gameName;
    uint8_t maxPlayers;
};