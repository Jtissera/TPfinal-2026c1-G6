#include "npcAttackMessage.h"

NpcAttackMessage::NpcAttackMessage(uint32_t npcId, Direction direction)
    : npcId(npcId),
      direction(direction)
{
}

uint32_t NpcAttackMessage::getNpcId() const { return npcId; }
Direction NpcAttackMessage::getDirection() const { return direction; }

uint8_t NpcAttackMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_NPC_ATTACK);
}

void NpcAttackMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(npcId);
    writer.writeUint8(static_cast<uint8_t>(direction));
}