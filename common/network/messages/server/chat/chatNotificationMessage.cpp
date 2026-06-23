#include "chatNotificationMessage.h"

ChatNotificationMessage::ChatNotificationMessage(std::string text, ChatMsgType type)
    : text(std::move(text)),
      type(type)
{
}

const std::string &ChatNotificationMessage::getText() const { return text; }
ChatMsgType ChatNotificationMessage::getMsgType() const { return type; }

uint8_t ChatNotificationMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_CHAT_MESSAGE);
}

void ChatNotificationMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeString(text);
    writer.writeUint8(static_cast<uint8_t>(type));
}