#include "connectMessage.h"

ConnectMessage::ConnectMessage(uint8_t protocolVersion, std::string username) : protocolVersion(protocolVersion), username(std::move(username)) {}

uint8_t ConnectMessage::version() const
{
    return protocolVersion;
}

uint8_t ConnectMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_CONNECT);
}

const std::string &ConnectMessage::getUsername() const
{
    return username;
}

void ConnectMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint8(protocolVersion);
    writer.writeString(username);
}