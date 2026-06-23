#pragma once
#include "../../message.h"
#include "../../../protocol/clientOpCode.h"
#include "../../../protocol/packetWriter.h"
#include <cstdint>
#include <string>

class InteractNpcMessage : public Message
{
public:
    InteractNpcMessage(uint32_t npcId, std::string cmd);

    uint32_t getNpcId() const;
    const std::string &getCmd() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t npcId;
    std::string cmd;
};