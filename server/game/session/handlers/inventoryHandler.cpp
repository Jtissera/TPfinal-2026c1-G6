#include "inventoryHandler.h"

InventoryHandler::InventoryHandler(const toml::table &config)
    : formulas(config) {}

void InventoryHandler::sendInventory(uint32_t clientId,
                                     Player &player,
                                     Monitor &monitor)
{
    monitor.sendTo(clientId,
                   std::make_shared<const InventoryUpdateMessage>(
                       player.getInventory().getItems(),
                       player.getInventory().getInventorySlots(),
                       player.getInventory().getEquippedArray()));
}

void InventoryHandler::sendStats(uint32_t clientId,
                                 Player &player,
                                 Monitor &monitor)
{
    monitor.sendTo(clientId,
                   std::make_shared<const PlayerStatsMessage>(
                       player.getLevel(),
                       player.getHp(), player.getMaxHp(),
                       player.getMana(), player.getMaxMana(),
                       player.getExp(),
                       formulas.calcExpLimit(player.getLevel()),
                       player.getGold()));
}

void InventoryHandler::handlePickItem(uint32_t clientId,
                                      const Message &msg,
                                      GameWorld &world,
                                      Monitor &monitor)
{
    Player &player = world.getPlayer(clientId);
    const PickItemMessage &pickMsg = static_cast<const PickItemMessage &>(msg);

    if (pickMsg.getIsGold())
    {
        std::optional<uint32_t> gold = world.pickGoldById(pickMsg.getInstanceId());
        if (gold.has_value())
        {
            player.addGold(*gold);
            sendStats(clientId, player, monitor);
            monitor.broadcast(std::make_shared<const ItemPickedMessage>(
                clientId, pickMsg.getInstanceId()));
        }
        return;
    }

    if (!player.getInventory().canAddItem())
    {
        return;
    }

    std::optional<Item> item = world.pickItemById(pickMsg.getInstanceId());
    if (item.has_value() && player.getInventory().addItem(std::move(*item)))
    {
        sendInventory(clientId, player, monitor);
        monitor.broadcast(std::make_shared<const ItemPickedMessage>(
            clientId, pickMsg.getInstanceId()));
    }
}

void InventoryHandler::handleDropItem(uint32_t clientId,
                                      const Message &msg,
                                      GameWorld &world,
                                      Monitor &monitor)
{
    const DropItemMessage &dropMsg = static_cast<const DropItemMessage &>(msg);
    Player &player = world.getPlayer(clientId);

    std::optional<Item> removed =
        player.getInventory().removeItem(dropMsg.getItemId());
    if (removed.has_value())
    {
        world.addItemOnGround(
            std::move(*removed), player.getTileX(), player.getTileY());
        sendInventory(clientId, player, monitor);
    }
}

void InventoryHandler::handleEquipItem(uint32_t clientId,
                                       const Message &msg,
                                       GameWorld &world,
                                       Monitor &monitor)
{
    const EquipItemMessage &equipMsg =
        static_cast<const EquipItemMessage &>(msg);
    const uint32_t itemInstanceId = equipMsg.getItemInstanceId();

    if (!world.hasPlayer(clientId))
    {
        return;
    }

    Player &player = world.getPlayer(clientId);
    const bool equipped = player.getInventory().equipItem(itemInstanceId);

    sendInventory(clientId, player, monitor);

    if (equipped)
    {
        monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(
            clientId, buildEquipmentDtoFromPlayer(player)));
    }
}

void InventoryHandler::handleUnequipSlot(uint32_t clientId,
                                         const Message &msg,
                                         GameWorld &world,
                                         Monitor &monitor)
{
    const UnequipSlotMessage &unequipMsg =
        static_cast<const UnequipSlotMessage &>(msg);
    const EquipSlot slot = unequipMsg.getSlot();

    if (!world.hasPlayer(clientId))
    {
        return;
    }

    Player &player = world.getPlayer(clientId);
    player.getInventory().unequipSlot(slot);

    sendInventory(clientId, player, monitor);
    monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(
        clientId, buildEquipmentDtoFromPlayer(player)));
}

void InventoryHandler::handleUseItem(uint32_t clientId,
                                     const Message &msg,
                                     GameWorld &world,
                                     Monitor &monitor)
{
    const UseItemMessage &useMsg = static_cast<const UseItemMessage &>(msg);
    const uint32_t itemInstanceId = useMsg.getItemInstanceId();

    if (!world.hasPlayer(clientId))
    {
        return;
    }

    Player &player = world.getPlayer(clientId);
    const Item *item = player.getInventory().findItem(itemInstanceId);

    if (item == nullptr)
    {
        sendInventory(clientId, player, monitor);
        return;
    }

    bool used = false;
    if (item->slot == ItemSlot::CONSUMABLE)
    {
        if (item->stats.healAmount > 0)
        {
            player.heal(item->stats.healAmount);
            used = true;
        }
        if (item->stats.manaAmount > 0)
        {
            player.restoreMana(item->stats.manaAmount);
            used = true;
        }
    }

    if (!used)
    {
        sendInventory(clientId, player, monitor);
        sendStats(clientId, player, monitor);
        return;
    }

    player.getInventory().removeItem(itemInstanceId);
    sendStats(clientId, player, monitor);
    sendInventory(clientId, player, monitor);
}