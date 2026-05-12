#pragma once

#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"

class ConnectOkMessage : public Message
{
public:
    uint8_t opCode() const override;

    void serializeBody(PacketWriter &writer) const override;
};