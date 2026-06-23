#pragma once
#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include <cstdint>

class ListGamesMessage : public Message
{
public:
    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;
};