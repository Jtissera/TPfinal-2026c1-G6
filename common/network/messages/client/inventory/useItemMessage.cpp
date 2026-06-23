#include "useItemMessage.h"

UseItemMessage::UseItemMessage(uint32_t itemInstanceId)
    : itemInstanceId(itemInstanceId)
{
}

uint32_t UseItemMessage::getItemInstanceId() const
{
    return itemInstanceId;
}

uint8_t UseItemMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_USE_ITEM);
}

void UseItemMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(itemInstanceId);
}