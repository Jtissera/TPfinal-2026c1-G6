#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include <cstdint>
#include <string>

enum class ChatMsgType : uint8_t
{
    GENERAL = 0,
    PRIVATE = 1,
    DAMAGE_DEALT = 2,
    DAMAGE_TAKEN = 3,
    INFO = 4,
    CLAN = 5,
};

class ChatNotificationMessage : public Message
{
public:
    ChatNotificationMessage(std::string text, ChatMsgType type);

    const std::string &getText() const;
    ChatMsgType getMsgType() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    std::string text;
    ChatMsgType type;
};