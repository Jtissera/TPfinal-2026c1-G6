#include "playerEquipmentUpdateMessage.h"



PlayerEquipmentUpdateMessage::PlayerEquipmentUpdateMessage(uint32_t playerId,EquipmentDto equipment): playerId(playerId),equipment(equipment) {}

uint8_t PlayerEquipmentUpdateMessage::opCode() const {
    return static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_EQUIPMENT_UPDATE);
}

void PlayerEquipmentUpdateMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint32(playerId);

    writer.writeUint32(equipment.weaponCatalogId);
    writer.writeUint32(equipment.armorCatalogId);
    writer.writeUint32(equipment.helmetCatalogId);
    writer.writeUint32(equipment.shieldCatalogId);
}

uint32_t PlayerEquipmentUpdateMessage::getPlayerId() const {
    return playerId;
}

const EquipmentDto& PlayerEquipmentUpdateMessage::getEquipment() const {
    return equipment;
}