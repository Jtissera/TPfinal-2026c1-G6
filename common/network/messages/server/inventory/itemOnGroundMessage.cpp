#include "itemOnGroundMessage.h"

ItemOnGroundMessage::ItemOnGroundMessage(Item item, int x, int y)
    : item(std::move(item)),
      x(x),
      y(y)
{
}

const Item &ItemOnGroundMessage::getItem() const { return item; }
int ItemOnGroundMessage::getX() const { return x; }
int ItemOnGroundMessage::getY() const { return y; }

uint8_t ItemOnGroundMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_ITEM_ON_GROUND);
}

void ItemOnGroundMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(item.instanceId);
    writer.writeUint32(item.catalogId);
    writer.writeString(item.typeName);
    writer.writeUint16(static_cast<uint16_t>(x));
    writer.writeUint16(static_cast<uint16_t>(y));
}