#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"
#include <cstdint>
#include <string>

class ChatMessage : public Message
{
public:
    explicit ChatMessage(std::string text, uint32_t targetId = 0);

    const std::string &getText() const;
    uint32_t getTargetId() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::string text;
    uint32_t targetId;
};