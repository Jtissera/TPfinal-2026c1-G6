#include "connectOKMessage.h"

uint8_t ConnectOkMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_CONNECT_OK);
}

void ConnectOkMessage::serializeBody(PacketWriter &writer) const
{
    (void)writer;
}