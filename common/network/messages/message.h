#pragma once
#include "../protocol/packetWriter.h"
#include <cstdint>

class Message
{
public:
    virtual ~Message() = default;

    virtual uint8_t opCode() const = 0;
    virtual void serializeBody(PacketWriter &writer) const = 0;
};