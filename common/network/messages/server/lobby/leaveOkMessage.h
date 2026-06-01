#pragma once

#include <cstdint>
#include <string>

#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include "../../message.h"

class LeaveOkMessage : public Message {
public:
  LeaveOkMessage();

  uint8_t opCode() const override;
  void serializeBody(PacketWriter &writer) const override;
};