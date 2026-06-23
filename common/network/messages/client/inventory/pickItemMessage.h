#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"
#include <cstdint>

class PickItemMessage : public Message
{
public:
    PickItemMessage(uint32_t instanceId, bool isGold);

    uint32_t getInstanceId() const;
    bool getIsGold() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t instanceId;
    bool isGold;
};