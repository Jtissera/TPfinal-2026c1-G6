#include "itemPickedMessage.h"

ItemPickedMessage::ItemPickedMessage(uint32_t clientId, uint32_t itemId)
    : clientId(clientId),
      itemId(itemId)
{
}

uint32_t ItemPickedMessage::getClientId() const { return clientId; }
uint32_t ItemPickedMessage::getItemId() const { return itemId; }

uint8_t ItemPickedMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_ITEM_PICKED);
}

void ItemPickedMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(clientId);
    writer.writeUint32(itemId);
}