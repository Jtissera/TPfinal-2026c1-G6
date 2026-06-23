#include "resurrectHandler.h"

ResurrectHandler::ResurrectHandler(const toml::table &config)
    : formulas(config) {}

void ResurrectHandler::sendStats(uint32_t clientId,
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

void ResurrectHandler::handleMeditate(uint32_t clientId,
                                      const Message &,
                                      GameWorld &world,
                                      Monitor &monitor)
{
    Player &player = world.getPlayer(clientId);
    if (player.isMeditating())
    {
        player.stopMeditating();
    }
    else
    {
        player.startMeditating();
    }
    sendStats(clientId, player, monitor);
}

void ResurrectHandler::handleResurrect(uint32_t clientId,
                                       const Message &,
                                       GameWorld &world,
                                       Monitor &monitor)
{
    Player &player = world.getPlayer(clientId);

    if (!player.isGhost())
    {
        return;
    }

    const uint16_t reviveTileX = static_cast<uint16_t>(player.getTileX());
    const uint16_t reviveTileY = static_cast<uint16_t>(player.getTileY());

    world.resurrectPlayer(clientId, reviveTileX, reviveTileY);

    monitor.broadcast(std::make_shared<const PlayerResurrectedMessage>(
        clientId, reviveTileX, reviveTileY));

    monitor.broadcast(std::make_shared<const EntityMoveMessage>(
        static_cast<uint32_t>(clientId),
        world.getPixelX(clientId),
        world.getPixelY(clientId),
        Direction::DOWN,
        false));

    sendStats(clientId, player, monitor);
}