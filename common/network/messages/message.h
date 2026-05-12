#pragma once

#include <string>
#include <cstdint>

#include "../protocol/packetWriter.h"

class Message
{

public:
    virtual ~Message() = default;

    virtual void serializeBody(PacketWriter &writer) const = 0;
    virtual uint8_t opCode() const = 0;
};