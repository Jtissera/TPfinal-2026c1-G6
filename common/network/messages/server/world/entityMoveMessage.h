#pragma once
#include "../../../../../game/position.h"
#include "../../../message.h"

class EntityMoveMessage : public Message {
public:
  EntityMoveMessage(uint32_t entityId, uint16_t x, uint16_t y);
  uint32_t getEntityId() const;
  Position getPosition() const;
  uint8_t opCode() const override;
  void serializeBody(PacketWriter &writer) const override;

private:
  uint32_t entityId;
  Position position;
};