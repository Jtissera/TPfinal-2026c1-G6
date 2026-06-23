#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>
#include <string>

class CombatLogMessage : public Message
{
public:
    explicit CombatLogMessage(std::string text);

    const std::string &getText() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::string text;
};