#pragma once
#include "../../message.h"
#include "../../../protocol/packetWriter.h"
#include "../../../protocol/serverOpCode.h"
#include <cstdint>
#include <string>

class NpcResponseMessage : public Message
{
public:
    explicit NpcResponseMessage(std::string text);

    const std::string &getText() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::string text;
};