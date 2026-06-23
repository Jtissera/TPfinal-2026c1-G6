#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include "common/dtos/gameTypes.h"
#include <cstdint>
#include <string>

class JoinOkMessage : public Message
{
public:
    JoinOkMessage(uint32_t gameId, std::string gameName, PlayerDto playerDto);

    uint32_t getGameId() const;
    const std::string &getGameName() const;
    const PlayerDto &getPlayerDto() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t gameId;
    std::string gameName;
    PlayerDto playerDto;
};