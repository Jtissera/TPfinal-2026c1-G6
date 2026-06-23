#include "chatMessage.h"

ChatMessage::ChatMessage(std::string text, uint32_t targetId)
    : text(std::move(text)),
      targetId(targetId)
{
}

const std::string &ChatMessage::getText() const { return text; }
uint32_t ChatMessage::getTargetId() const { return targetId; }

uint8_t ChatMessage::opCode() const
{
    return static_cast<uint8_t>(ClientOpCode::MSG_CHAT);
}

void ChatMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeString(text);
    writer.writeUint32(targetId);
}