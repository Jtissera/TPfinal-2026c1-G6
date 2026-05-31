#pragma once
#include "../../message.h"
#include "../../../protocol/serverOpCode.h"
#include "../../../protocol/packetWriter.h"

class CreateOkMessage : public Message 
{
public:
    uint8_t opCode() const override;

    void serializeBody(PacketWriter &writer) const override;
};