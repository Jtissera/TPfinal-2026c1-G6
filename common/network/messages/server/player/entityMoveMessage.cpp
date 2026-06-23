#include "entityMoveMessage.h"

EntityMoveMessage::EntityMoveMessage(uint32_t entityId,
                                     uint16_t x,
                                     uint16_t y,
                                     Direction direction,
                                     bool moving)
    : entityId(entityId),
      x(x),
      y(y),
      direction(direction),
      moving(moving)
{
}

uint32_t EntityMoveMessage::getId() const { return entityId; }
uint16_t EntityMoveMessage::getX() const { return x; }
uint16_t EntityMoveMessage::getY() const { return y; }
Direction EntityMoveMessage::getDirection() const { return direction; }
bool EntityMoveMessage::isMoving() const { return moving; }

uint8_t EntityMoveMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE);
}

void EntityMoveMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(entityId);
    writer.writeUint16(x);
    writer.writeUint16(y);
    writer.writeUint8(static_cast<uint8_t>(direction));
    writer.writeUint8(moving ? 1 : 0);
}