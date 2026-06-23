#pragma once
#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include <cstdint>

class ResurrectMessage : public Message
{
public:
    ResurrectMessage() = default;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;
};