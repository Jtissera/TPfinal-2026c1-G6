

#include "EntityMoveMessage.h"


EntityMoveMessage::EntityMoveMessage(uint8_t id, uint16_t x, uint16_t y) : entityId(id), x(x), y(y) {}

uint8_t EntityMoveMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE);
}

void EntityMoveMessage::serializeBody(PacketWriter &writer) const {
    writer.writeUint8(entityId);
    writer.writeUint16(x);
    writer.writeUint16(y);

}

uint8_t EntityMoveMessage::getId() const {
    return entityId;
}

uint16_t EntityMoveMessage::getX() const {
    return x;
}

uint16_t EntityMoveMessage::getY() const {
    return y;
}
