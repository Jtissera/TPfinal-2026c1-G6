#pragma once
#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include "../../../../dtos/gameTypes.h"
#include <cstdint>

class MoveMessage : public Message
{
public:
    MoveMessage(Direction dir, bool moving);

    Direction getDirection() const;
    bool isMoving() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    Direction dir;
    bool moving;
};