#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include "common/dtos/gameTypes.h"
#include <cstdint>

class EntityMoveMessage : public Message
{
public:
    EntityMoveMessage(uint32_t entityId,
                      uint16_t x,
                      uint16_t y,
                      Direction direction,
                      bool moving);

    uint32_t getId() const;
    uint16_t getX() const;
    uint16_t getY() const;
    Direction getDirection() const;
    bool isMoving() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t entityId;
    uint16_t x;
    uint16_t y;
    Direction direction;
    bool moving;
};