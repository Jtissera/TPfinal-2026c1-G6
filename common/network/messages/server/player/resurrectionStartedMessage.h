#pragma once

#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>

class ResurrectionStartedMessage : public Message
{
public:
    explicit ResurrectionStartedMessage(uint32_t delayMs) : delayMs(delayMs) {}

    uint32_t getDelayMs() const { return delayMs; }

    uint8_t opCode() const override
    {
        return static_cast<uint8_t>(ServerOpCode::MSG_RESURRECTION_STARTED);
    }

    void serializeBody(PacketWriter &writer) const override
    {
        writer.writeUint32(delayMs);
    }

private:
    uint32_t delayMs;
};