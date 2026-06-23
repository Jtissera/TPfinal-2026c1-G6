#include "attackMessage.h"

AttackMessage::AttackMessage(uint32_t targetId)
    : targetId(targetId)
{
}

uint32_t AttackMessage::getTargetId() const
{
    return targetId;
}

uint8_t AttackMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_ATTACK);
}

void AttackMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(targetId);
}