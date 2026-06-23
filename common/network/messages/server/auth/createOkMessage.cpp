#include "createOkMessage.h"

uint8_t CreateOkMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_CREATE_OK);
}

void CreateOkMessage::serializeBody(PacketWriter &writer) const
{
    (void)writer;
}