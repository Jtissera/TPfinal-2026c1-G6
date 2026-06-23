#include "dropItemMessage.h"

DropItemMessage::DropItemMessage(uint32_t itemId)
    : itemId(itemId)
{
}

uint32_t DropItemMessage::getItemId() const
{
    return itemId;
}

uint8_t DropItemMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_DROP_ITEM);
}

void DropItemMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(itemId);
}