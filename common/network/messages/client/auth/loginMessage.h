#pragma once
#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include <cstdint>
#include <string>

class LoginMessage : public Message
{
public:
  explicit LoginMessage(std::string characterName);

  const std::string &getCharacterName() const;

  uint8_t opCode() const override;
  void serializeBody(PacketWriter &writer) const override;

private:
  std::string characterName;
};