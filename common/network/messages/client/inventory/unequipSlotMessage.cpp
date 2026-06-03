
#include "unequipSlotMessage.h"

UnequipSlotMessage::UnequipSlotMessage(EquipSlot slot)
    : slot(slot) {
}

uint8_t UnequipSlotMessage::opCode() const {
    return static_cast<uint8_t>(ClientOpCode::MSG_UNEQUIP_SLOT);
}

void UnequipSlotMessage::serializeBody(PacketWriter& writer) const {
    writer.writeUint8(static_cast<uint8_t>(slot));
}

EquipSlot UnequipSlotMessage::getSlot() const {
    return slot;
}
