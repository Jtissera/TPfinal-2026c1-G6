#pragma once

#include <cstdint>
#include <string>

#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include "../../message.h"

class LoginMessage : public Message {
public:
  explicit LoginMessage(std::string characterName);

  uint8_t opCode() const override;
  void serializeBody(PacketWriter &writer) const override;
  const std::string &getCharacterName() const;

private:
  std::string characterName;
};