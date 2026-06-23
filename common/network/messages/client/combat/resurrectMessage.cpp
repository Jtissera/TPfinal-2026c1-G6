#include "resurrectMessage.h"

uint8_t ResurrectMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT);
}

void ResurrectMessage::serializeBody(PacketWriter &writer) const
{
    (void)writer;
}