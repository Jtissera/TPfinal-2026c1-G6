#pragma once
#include "../../../../../game/direction.h"
#include "../../../../../network/protocol/clientOpCode.h"
#include "../../message.h"

class MoveMessage : public Message {
public:
  explicit MoveMessage(Direction dir);
  Direction getDirection() const;
  uint8_t opCode() const override;
  void serializeBody(PacketWriter &writer) const override;

private:
  Direction direction;
};