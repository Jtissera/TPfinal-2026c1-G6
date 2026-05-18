#include "entityMoveMessage.h"
#include "../../../../../network/protocol/serverOpCode.h"

EntityMoveMessage::EntityMoveMessage(uint32_t entityId, uint16_t x, uint16_t y)
    : entityId(entityId), position({x, y}) {}

uint32_t EntityMoveMessage::getEntityId() const { return entityId; }
Position EntityMoveMessage::getPosition() const { return position; }

uint8_t EntityMoveMessage::opCode() const {
  return static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE);
}

void EntityMoveMessage::serializeBody(PacketWriter &writer) const {
  writer.writeUint32(entityId);
  writer.writeUint16(position.x);
  writer.writeUint16(position.y);
}