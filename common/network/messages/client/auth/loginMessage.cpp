#include "loginMessage.h"

LoginMessage::LoginMessage(std::string characterName)
    : characterName(std::move(characterName)) {}

uint8_t LoginMessage::opCode() const {
  return static_cast<uint8_t>(ClientOpCode::MSG_LOGIN);
}

void LoginMessage::serializeBody(PacketWriter &writer) const {
  writer.writeString(characterName);
}

const std::string &LoginMessage::getCharacterName() const {
  return characterName;
}