#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"

class AttackMessage : public Message {
  uint32_t targetId;

public:
  explicit AttackMessage(uint32_t targetId);
  uint32_t getTargetId() const;
  uint8_t opCode() const override;
  void serializeBody(PacketWriter &writer) const override;
};