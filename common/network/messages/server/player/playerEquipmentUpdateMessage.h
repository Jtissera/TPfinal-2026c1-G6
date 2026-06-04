
#ifndef TALLER_TP_PLAYEREQUIPMENTUPDATEMESSAGE_H
#define TALLER_TP_PLAYEREQUIPMENTUPDATEMESSAGE_H
#include <cstdint>
#include "common/network/protocol/packetWriter.h"
#include "common/network/protocol/serverOpCode.h"
#include "common/dtos/equipmentDto.h"
#include "common/network/messages/message.h"

class PlayerEquipmentUpdateMessage : public Message {
private:
    uint32_t playerId;
    EquipmentDto equipment;

public:
    PlayerEquipmentUpdateMessage(uint32_t playerId, EquipmentDto equipment);

    uint8_t opCode() const override;

    void serializeBody(PacketWriter& writer) const override;

    uint32_t getPlayerId() const;

    const EquipmentDto& getEquipment() const;
};

#endif //TALLER_TP_PLAYEREQUIPMENTUPDATEMESSAGE_H
