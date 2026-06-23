#include "cheatHandler.h"

CheatHandler::CheatHandler(const toml::table &config)
    : formulas(config) {}

void CheatHandler::sendStats(uint32_t clientId,
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

void CheatHandler::sendInventory(uint32_t clientId,
                                 Player &player,
                                 Monitor &monitor)
{
    monitor.sendTo(clientId,
                   std::make_shared<const InventoryUpdateMessage>(
                       player.getInventory().getItems(),
                       player.getInventory().getInventorySlots(),
                       player.getInventory().getEquippedArray()));
}

void CheatHandler::sendDeath(uint32_t clientId,
                             Player &player,
                             Monitor &monitor)
{
    sendInventory(clientId, player, monitor);
    sendStats(clientId, player, monitor);
    monitor.broadcast(std::make_shared<const PlayerDiedMessage>(clientId));
    monitor.sendTo(clientId,
                   std::make_shared<const PlayerDiedMessage>(clientId));
}

void CheatHandler::sendLevelUpIfNeeded(uint32_t clientId,
                                       Player &player,
                                       Monitor &monitor)
{
    if (!player.checkAndClearLevelUp())
    {
        return;
    }
    monitor.broadcast(std::make_shared<const LevelUpMessage>(
        clientId, static_cast<uint8_t>(player.getLevel())));
    monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
        clientId,
        static_cast<uint16_t>(player.getHp()),
        static_cast<uint16_t>(player.getMaxHp())));
}

void CheatHandler::handle(uint32_t clientId,
                          const Message &msg,
                          GameWorld &world,
                          Monitor &monitor)
{
    const CheatMessage &cheatMsg = static_cast<const CheatMessage &>(msg);
    Player &player = world.getPlayer(clientId);

    switch (cheatMsg.getCheat())
    {
    case CheatType::INFINITE_HP:
        player.toggleInfiniteHp();
        sendStats(clientId, player, monitor);
        break;

    case CheatType::INFINITE_MANA:
        player.toggleInfiniteMana();
        sendStats(clientId, player, monitor);
        break;

    case CheatType::DIE:
    {
        if (!player.isAlive() || player.isGhost())
        {
            return;
        }
            DeathResult deathResult =
            world.handlePlayerDeath(clientId, 0);
        if (deathResult.excessGold > 0)
        {
            monitor.broadcast(std::make_shared<const GoldOnGroundMessage>(
                deathResult.goldInstanceId,
                deathResult.excessGold,
                deathResult.tileX,
                deathResult.tileY));
        }
        for (const Item &item : deathResult.droppedItems)
        {
            monitor.broadcast(std::make_shared<const ItemOnGroundMessage>(
                item, deathResult.tileX, deathResult.tileY));
        }
        sendDeath(clientId, player, monitor);
        break;
    }

    case CheatType::ADD_GOLD:
        player.addGold(1000);
        sendInventory(clientId, player, monitor);
        sendStats(clientId, player, monitor);
        break;

    case CheatType::LEVEL_UP:
    {
        uint32_t currentExp = player.getExp();
        uint32_t limit = formulas.calcExpLimit(player.getLevel());
        uint32_t toNextLevel =
            (limit > currentExp) ? (limit - currentExp) : 1;
        world.giveExperience(clientId, toNextLevel);
        sendStats(clientId, player, monitor);
        sendLevelUpIfNeeded(clientId, player, monitor);
        break;
    }
    }
}