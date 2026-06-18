#include "loginOkMessage.h"

uint8_t LoginOkMessage::opCode() const {
  return static_cast<uint8_t>(ServerOpCode::MSG_LOGIN_OK);
}

void LoginOkMessage::serializeBody(PacketWriter &) const {
  // sin payload, el opcode solo alcanza
}