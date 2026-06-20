#pragma once
#include <cstdint>
#include "../../message.h"
#include "../../../../dtos/gameTypes.h" 
#include "../../../protocol/serverOpCode.h"

class NpcAttackMessage : public Message
{
public:
    NpcAttackMessage(uint32_t npcId, Direction direction);

    uint8_t opCode() const override;
    void serializeBody(PacketWriter& writer) const override;

    uint32_t getNpcId() const;
    Direction getDirection() const;

private:
    uint32_t npcId;
    Direction direction;
};