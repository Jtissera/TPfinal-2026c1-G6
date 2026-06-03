#include "equipItemMessage.h"

EquipItemMessage::EquipItemMessage(uint32_t itemInstanceId)
    : itemInstanceId(itemInstanceId) {
}

uint8_t EquipItemMessage::opCode() const {
    return static_cast<uint8_t>(ClientOpCode::MSG_EQUIP_ITEM);
}

void EquipItemMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint32(itemInstanceId);
}

uint32_t EquipItemMessage::getItemInstanceId() const {
    return itemInstanceId;
}