#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include <cstdint>

class LoginOkMessage : public Message
{
public:
  uint8_t opCode() const override;
  void serializeBody(PacketWriter &writer) const override;
};