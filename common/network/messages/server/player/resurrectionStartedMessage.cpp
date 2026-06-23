#include "resurrectionStartedMessage.h"

ResurrectionStartedMessage::ResurrectionStartedMessage(uint32_t delayMs)
    : delayMs(delayMs)
{
}

uint32_t ResurrectionStartedMessage::getDelayMs() const
{
    return delayMs;
}

uint8_t ResurrectionStartedMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_RESURRECTION_STARTED);
}

void ResurrectionStartedMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint32(delayMs);
}