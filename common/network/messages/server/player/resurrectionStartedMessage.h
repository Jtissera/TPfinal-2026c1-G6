#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>

class ResurrectionStartedMessage : public Message
{
public:
    explicit ResurrectionStartedMessage(uint32_t delayMs);

    uint32_t getDelayMs() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t delayMs;
};