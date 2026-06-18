#pragma once

#include "common/network/messages/message.h"
#include "common/network/protocol/serverOpCode.h"
#include <string>
#include <cstdint>

enum class ChatMsgType : uint8_t
{
    GENERAL = 0,      // Blanco   — chat público
    PRIVATE = 1,      // Amarillo — mensaje privado
    DAMAGE_DEALT = 2, // Rojo     — daño causado
    DAMAGE_TAKEN = 3, // Verde    — daño recibido
    INFO = 4,         // Celeste  — sistema, NPC, errores leves
    CLAN = 5,         // Cyan/Esmeralda — mensajes de clan
};

class ChatNotificationMessage : public Message
{
public:
    ChatNotificationMessage(std::string text, ChatMsgType type)
        : text(std::move(text)), type(type) {}

    const std::string &getText() const { return text; }
    ChatMsgType getMsgType() const { return type; }

    uint8_t opCode() const override
    {
        return static_cast<uint8_t>(ServerOpCode::MSG_CHAT_MESSAGE);
    }

    void serializeBody(PacketWriter &writer) const override
    {
        writer.writeString(text);
        writer.writeUint8(static_cast<uint8_t>(type));
    }

private:
    std::string text;
    ChatMsgType type;
};
