#pragma once

#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include "../../message.h"

class LoginOkMessage : public Message {
public:
  uint8_t opCode() const override;
  void serializeBody(PacketWriter &writer) const override;
};