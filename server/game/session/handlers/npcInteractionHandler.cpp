#include "npcInteractionHandler.h"

NpcInteractionHandler::NpcInteractionHandler(const toml::table &config)
    : formulas(config) {}

void NpcInteractionHandler::sendStats(uint32_t clientId,
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

void NpcInteractionHandler::sendInventory(uint32_t clientId,
                                          Player &player,
                                          Monitor &monitor)
{
    monitor.sendTo(clientId,
                   std::make_shared<const InventoryUpdateMessage>(
                       player.getInventory().getItems(),
                       player.getInventory().getInventorySlots(),
                       player.getInventory().getEquippedArray()));
}

void NpcInteractionHandler::handle(uint32_t clientId,
                                   const Message &msg,
                                   GameWorld &world,
                                   Monitor &monitor)
{
    const InteractNpcMessage &interactMsg =
        static_cast<const InteractNpcMessage &>(msg);
    Player &player = world.getPlayer(clientId);

    if (player.isGhost())
    {
        std::optional<CityCommand> cmd =
            cityCommandParser.parse(interactMsg.getCmd());
        if (!cmd.has_value() || cmd->type != CityCommand::Type::RESURRECT)
        {
            monitor.sendTo(clientId,
                           std::make_shared<const ErrorMessage>(
                               "A ghost cannot interact."));
            return;
        }
        CityResult result =
            world.handleRemoteResurrect(clientId);
        monitor.sendTo(clientId,
                       std::make_shared<const NpcResponseMessage>(result.message));
        return;
    }

    const int px = player.getTileX();
    const int py = player.getTileY();

    std::optional<NpcType> npcType;
    for (int dx = -1; dx <= 1 && !npcType.has_value(); dx++)
    {
        for (int dy = -1; dy <= 1 && !npcType.has_value(); dy++)
        {
            npcType = world.getNpcTypeAtTile(px + dx, py + dy);
        }
    }

    if (!npcType.has_value())
    {
        monitor.sendTo(clientId,
                       std::make_shared<const ErrorMessage>("No NPC nearby."));
        return;
    }

    std::optional<CityCommand> cmd =
        cityCommandParser.parse(interactMsg.getCmd());
    if (!cmd.has_value())
    {
        monitor.sendTo(clientId,
                       std::make_shared<const ErrorMessage>("Invalid command."));
        return;
    }

    CityResult result =
        world.handleCityInteraction(clientId, *npcType, *cmd);
    monitor.sendTo(clientId,
                   std::make_shared<const NpcResponseMessage>(result.message));
    sendStats(clientId, player, monitor);
    if (result.ok)
    {
        sendInventory(clientId, player, monitor);
    }
}