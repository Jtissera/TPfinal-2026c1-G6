#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/clientOpCode.h"
#include "common/network/protocol/packetWriter.h"
#include "server/game/items/EquipSlot.h"
#include <cstdint>

class UnequipSlotMessage : public Message
{
public:
    explicit UnequipSlotMessage(EquipSlot slot);

    EquipSlot getSlot() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    EquipSlot slot;
};