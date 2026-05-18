#include "moveMessage.h"

MoveMessage::MoveMessage(Direction dir) : direction(dir) {}

Direction MoveMessage::getDirection() const { return direction; }

uint8_t MoveMessage::opCode() const {
  return static_cast<uint8_t>(ClientOpCode::MSG_MOVE);
}

void MoveMessage::serializeBody(PacketWriter &writer) const {
  writer.writeUint8(static_cast<uint8_t>(direction));
}