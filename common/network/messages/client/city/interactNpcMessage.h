#pragma once
#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include <cstdint>
#include <string>

class InteractNpcMessage : public Message
{
public:
    InteractNpcMessage(uint32_t npcId, std::string cmd)
        : npcId(npcId), cmd(std::move(cmd)) {}

    uint8_t opCode() const override
    {
        return static_cast<uint8_t>(ClientOpCode::MSG_INTERACT_NPC);
    }

    void serializeBody(PacketWriter &writer) const override
    {
        writer.writeUint32(npcId);
        writer.writeString(cmd);
    }

    uint32_t getNpcId() const { return npcId; }
    const std::string &getCmd() const { return cmd; }

private:
    uint32_t npcId;
    std::string cmd;
};