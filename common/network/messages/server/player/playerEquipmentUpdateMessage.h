#pragma once
#include "common/network/messages/message.h"
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include "common/dtos/equipmentDto.h"
#include <cstdint>

class PlayerEquipmentUpdateMessage : public Message
{
public:
    PlayerEquipmentUpdateMessage(uint32_t playerId, EquipmentDto equipment);

    uint32_t getPlayerId() const;
    const EquipmentDto &getEquipment() const;

    uint8_t opCode() const override;
    void serializeBody(PacketWriter &writer) const override;

private:
    uint32_t playerId;
    EquipmentDto equipment;
};