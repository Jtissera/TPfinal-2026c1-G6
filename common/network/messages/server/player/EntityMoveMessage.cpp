

#include "EntityMoveMessage.h"

EntityMoveMessage::EntityMoveMessage(uint32_t id,int16_t x,int16_t y,Direction direction,bool moving)
    : entityId(id),
      x(x),
      y(y),
      direction(direction),
      moving(moving) {
}

uint8_t EntityMoveMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE);
}

void EntityMoveMessage::serializeBody(PacketWriter& writer) const {
    // Id de la entidad que se movió.
    writer.writeUint32(entityId);

    // Posición final validada por servidor.
    writer.writeUint16(x);
    writer.writeUint16(y);

    // Dirección aceptada por servidor.
    writer.writeUint8(static_cast<uint8_t>(direction));

    // Estado de movimiento. Por ahora true cuando el movimiento fue válido.
    writer.writeUint8(moving ? 1 : 0);
}

uint32_t EntityMoveMessage::getId() const {
    return entityId;
}

int16_t EntityMoveMessage::getX() const {
    return x;
}

int16_t EntityMoveMessage::getY() const {
    return y;
}

Direction EntityMoveMessage::getDirection() const {
    return direction;
}

bool EntityMoveMessage::isMoving() const {
    return moving;
}