#pragma once

#include <cstdint>

#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"

class EquipItemMessage : public Message {
private:
    uint32_t itemInstanceId;

public:
    explicit EquipItemMessage(uint32_t itemInstanceId);

    uint8_t opCode() const override;

    void serializeBody(PacketWriter& writer) const override;

    uint32_t getItemInstanceId() const;
};