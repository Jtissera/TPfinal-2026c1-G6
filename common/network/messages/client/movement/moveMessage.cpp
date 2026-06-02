#include "moveMessage.h"

MoveMessage::MoveMessage(Direction dir, bool moving): dir(dir),moving(moving) {}

uint8_t MoveMessage::opCode() const {
    return static_cast<uint8_t>(ClientOpCode::MSG_MOVE);
}

void MoveMessage::serializeBody(PacketWriter& writer) const {

    writer.writeUint8(static_cast<uint8_t>(dir));
    // Enviamos si está moviéndose.
    // 1 = caminando, 0 = detenido.
    writer.writeUint8(moving ? 1 : 0);
}

Direction MoveMessage::getDirection() const {
    return dir;
}

bool MoveMessage::isMoving() const {
    return moving;
}