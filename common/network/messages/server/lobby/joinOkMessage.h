#pragma once

#include <cstdint>
#include <string>

#include "../../message.h"
#include "../../../protocol/serverOpCode.h"
#include "../../../protocol/packetWriter.h"
#include "common/dtos/gameTypes.h"

class JoinOkMessage : public Message
{
public:
    JoinOkMessage(uint32_t gameId, std::string gameName, PlayerDto player_dto);

    uint32_t getGameId() const;
    const std::string &getGameName() const;

    // Devuelve el estado inicial real del jugador enviado por el servidor.
    const PlayerDto& getPlayerDto() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t gameId;
    std::string gameName;
    PlayerDto playerDto;
};
