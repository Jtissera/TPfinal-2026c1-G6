#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"
#include <cstdint>

class EquipItemMessage : public Message
{
public:
    explicit EquipItemMessage(uint32_t itemInstanceId);

    uint32_t getItemInstanceId() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t itemInstanceId;
};