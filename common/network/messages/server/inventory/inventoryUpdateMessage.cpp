#include "inventoryUpdateMessage.h"

InventoryUpdateMessage::InventoryUpdateMessage(
    std::vector<Item> items,
    std::array<uint32_t, INVENTORY_SLOT_COUNT> inventorySlots,
    std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> equipped)
    : items(std::move(items)),
      inventorySlots(inventorySlots),
      equipped(equipped)
{
}

const std::vector<Item> &InventoryUpdateMessage::getItems() const
{
    return items;
}

const std::array<uint32_t, InventoryUpdateMessage::INVENTORY_SLOT_COUNT> &
InventoryUpdateMessage::getInventorySlots() const
{
    return inventorySlots;
}

const std::array<uint32_t, static_cast<std::size_t>(EquipSlot::COUNT)> &
InventoryUpdateMessage::getEquipped() const
{
    return equipped;
}

uint8_t InventoryUpdateMessage::opCode() const
{
    return static_cast<uint8_t>(ServerOpCode::MSG_INVENTORY_UPDATE);
}

void InventoryUpdateMessage::serializeBody(PacketWriter &writer) const
{
    writer.writeUint8(static_cast<uint8_t>(items.size()));
    for (const Item &item : items)
    {
        writer.writeUint32(item.instanceId);
        writer.writeUint32(item.catalogId);
        writer.writeString(item.typeName);
        writer.writeUint8(static_cast<uint8_t>(item.slot));
    }
    writer.writeUint8(static_cast<uint8_t>(inventorySlots.size()));
    for (uint32_t id : inventorySlots)
    {
        writer.writeUint32(id);
    }
    for (uint32_t id : equipped)
    {
        writer.writeUint32(id);
    }
}