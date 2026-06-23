#include "errorMessage.h"

ErrorMessage::ErrorMessage(std::string reason)
    : reason(std::move(reason))
{
}

const std::string &ErrorMessage::getReason() const
{
    return reason;
}

uint8_t ErrorMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_ERROR);
}

void ErrorMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeString(reason);
}