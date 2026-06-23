#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include "common/dtos/gameTypes.h"
#include <cstdint>

class EntitySpawnMessage : public Message
{
public:
    explicit EntitySpawnMessage(PlayerDto playerDto);

    const PlayerDto &getPlayerDto() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    PlayerDto playerDto;
};