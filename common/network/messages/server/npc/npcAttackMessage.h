#pragma once
#include "../../message.h"
#include "../../../../dtos/gameTypes.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include <cstdint>

class NpcAttackMessage : public Message
{
public:
    NpcAttackMessage(uint32_t npcId, Direction direction);

    uint32_t getNpcId() const;
    Direction getDirection() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t npcId;
    Direction direction;
};