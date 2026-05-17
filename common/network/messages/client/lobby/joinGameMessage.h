#pragma once

#include <cstdint>

#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"

class JoinGameMessage : public Message
{
public:
    explicit JoinGameMessage(uint32_t gameId);

    uint32_t getGameId() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t gameId;
};