#pragma once
#include "../../../protocol/clientOpCode.h"
#include "../../message.h"

class LeaveGameMessage : public Message {
public:
  uint8_t opCode() const override;
  void serializeBody(PacketWriter &writer) const override;
};