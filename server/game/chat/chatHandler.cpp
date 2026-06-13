#include "chatHandler.h"

#include "common/network/messages/server/chat/chatNotificationMessage.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "server/game/stats/gameFormulas.h"
#include "server/city/cityCommandParser.h"
#include "server/city/cityResult.h"

void ChatHandler::sendChat(uint32_t clientId,
                           const std::string &text,
                           ChatMsgType type,
                           Monitor &monitor)
{
    monitor.sendTo(clientId,
                   std::make_shared<const ChatNotificationMessage>(text, type));
}

void ChatHandler::broadcastChat(const std::string &text,
                                ChatMsgType type,
                                Monitor &monitor)
{
    monitor.broadcast(
        std::make_shared<const ChatNotificationMessage>(text, type));
}

void ChatHandler::sendStats(uint32_t id, Player &p, Monitor &monitor)
{
    monitor.sendTo(id, std::make_shared<const PlayerStatsMessage>(
                           p.getLevel(), p.getHp(), p.getMaxHp(),
                           p.getMana(), p.getMaxMana(),
                           p.getExp(), 0u,
                           p.getGold()));
}

void ChatHandler::sendInventory(uint32_t id, Player &p, Monitor &monitor)
{
    monitor.sendTo(id, std::make_shared<const InventoryUpdateMessage>(
                           p.getInventory().getItems(),
                           p.getInventory().getInventorySlots(),
                           p.getInventory().getEquippedArray()));
}

bool ChatHandler::handle(uint32_t senderId,
                         const std::string &rawText,
                         uint32_t targetId,
                         GameWorld &world,
                         Monitor &monitor)
{
    auto parsed = ChatCommandParser::parse(rawText);
    if (!parsed)
        return false;

    switch (parsed->type)
    {
    case ChatInputType::GENERAL:
        handleGeneral(senderId, parsed->keyword, world, monitor);
        break;
    case ChatInputType::PRIVATE:
        handlePrivate(senderId, parsed->targetNick, parsed->keyword,
                      world, monitor);
        break;
    case ChatInputType::COMMAND:
        handleCommand(senderId, *parsed, targetId, world, monitor);
        break;
    }
    return true;
}

void ChatHandler::handleGeneral(uint32_t senderId,
                                const std::string &text,
                                GameWorld &world,
                                Monitor &monitor)
{
    const Player &sender = world.getPlayer(senderId);
    broadcastChat("[" + sender.getName() + "]: " + text,
                  ChatMsgType::GENERAL, monitor);
}

void ChatHandler::handlePrivate(uint32_t senderId,
                                const std::string &targetNick,
                                const std::string &text,
                                GameWorld &world,
                                Monitor &monitor)
{
    const Player &sender = world.getPlayer(senderId);

    std::optional<uint32_t> targetId = world.findPlayerIdByName(targetNick);
    if (!targetId)
    {
        sendChat(senderId,
                 "El jugador '" + targetNick + "' no está conectado.",
                 ChatMsgType::INFO, monitor);
        return;
    }

    sendChat(*targetId,
             "[Privado de " + sender.getName() + "]: " + text,
             ChatMsgType::PRIVATE, monitor);
    sendChat(senderId,
             "[Privado a " + targetNick + "]: " + text,
             ChatMsgType::PRIVATE, monitor);
}

void ChatHandler::handleCommand(uint32_t senderId,
                                const ParsedChatInput &parsed,
                                uint32_t targetId,
                                GameWorld &world,
                                Monitor &monitor)
{
    const std::string &cmd = parsed.keyword;
    const std::string &arg = parsed.argument;

    if (cmd == "meditar")
    {
        handleMeditate(senderId, world, monitor);
        return;
    }

    if (cmd == "tomar")
    {
        handlePickItem(senderId, world, monitor);
        return;
    }

    if (cmd == "tirar")
    {
        if (arg.empty())
        {
            sendChat(senderId, "Uso: /tirar <nombre_item>",
                     ChatMsgType::INFO, monitor);
            return;
        }
        handleDropItem(senderId, arg, world, monitor);
        return;
    }

    if (cmd == "resucitar")
    {
        Player &p = world.getPlayer(senderId);
        if (!p.isGhost())
        {
            sendChat(senderId, "No estás muerto.", ChatMsgType::INFO, monitor);
            return;
        }
        auto result = world.handleRemoteResurrect(senderId);
        sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
        return;
    }

    Player &player = world.getPlayer(senderId);
    int px = player.getTileX();
    int py = player.getTileY();

    std::optional<NpcType> npcType;
    for (int dx = -1; dx <= 1 && !npcType; dx++)
        for (int dy = -1; dy <= 1 && !npcType; dy++)
            npcType = world.getNpcTypeAtTile(px + dx, py + dy);

    if (cmd == "curar")
    {
        if (!npcType || *npcType != NpcType::PRIEST)
        {
            sendChat(senderId, "No hay ningún sacerdote cerca.",
                     ChatMsgType::INFO, monitor);
            return;
        }
        CityCommand cityCmd;
        cityCmd.type = CityCommand::Type::HEAL;
        auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
        sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
        if (result.ok)
        {
            sendStats(senderId, player, monitor);
        }
        return;
    }

    if (cmd == "comprar")
    {
        if (arg.empty())
        {
            sendChat(senderId, "Uso: /comprar <nombre_item>",
                     ChatMsgType::INFO, monitor);
            return;
        }
        if (!npcType)
        {
            sendChat(senderId, "No hay ningún NPC cerca.",
                     ChatMsgType::INFO, monitor);
            return;
        }
        CityCommand cityCmd;
        cityCmd.type = CityCommand::Type::BUY;
        cityCmd.itemName = arg;
        auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
        sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
        if (result.ok)
        {
            sendStats(senderId, player, monitor);
            sendInventory(senderId, player, monitor);
        }
        return;
    }

    if (cmd == "vender")
    {
        if (arg.empty())
        {
            sendChat(senderId, "Uso: /vender <nombre_item>",
                     ChatMsgType::INFO, monitor);
            return;
        }
        if (!npcType || *npcType != NpcType::MERCHANT)
        {
            sendChat(senderId, "No hay ningún comerciante cerca.",
                     ChatMsgType::INFO, monitor);
            return;
        }
        CityCommand cityCmd;
        cityCmd.type = CityCommand::Type::SELL;
        cityCmd.itemName = arg;
        auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
        sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
        if (result.ok)
        {
            sendStats(senderId, player, monitor);
            sendInventory(senderId, player, monitor);
        }
        return;
    }

    if (cmd == "depositar")
    {
        if (arg.empty())
        {
            sendChat(senderId,
                     "Uso: /depositar <nombre_item>  o  /depositar oro <cantidad>",
                     ChatMsgType::INFO, monitor);
            return;
        }
        if (!npcType || *npcType != NpcType::BANKER)
        {
            sendChat(senderId, "No hay ningún banquero cerca.",
                     ChatMsgType::INFO, monitor);
            return;
        }
        CityCommand cityCmd;
        cityCmd.type = CityCommand::Type::DEPOSIT;
        if (arg.size() > 4 && arg.substr(0, 4) == "oro ")
        {
            try
            {
                cityCmd.goldAmount = static_cast<uint32_t>(std::stoul(arg.substr(4)));
                cityCmd.isGold = true;
            }
            catch (...)
            {
                sendChat(senderId, "Cantidad inválida.", ChatMsgType::INFO, monitor);
                return;
            }
        }
        else
        {
            cityCmd.itemName = arg;
            cityCmd.isGold = false;
        }
        auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
        sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
        if (result.ok)
        {
            sendStats(senderId, player, monitor);
            sendInventory(senderId, player, monitor);
        }
        return;
    }

    if (cmd == "retirar")
    {
        if (arg.empty())
        {
            sendChat(senderId,
                     "Uso: /retirar <nombre_item>  o  /retirar oro <cantidad>",
                     ChatMsgType::INFO, monitor);
            return;
        }
        if (!npcType || *npcType != NpcType::BANKER)
        {
            sendChat(senderId, "No hay ningún banquero cerca.",
                     ChatMsgType::INFO, monitor);
            return;
        }
        CityCommand cityCmd;
        cityCmd.type = CityCommand::Type::WITHDRAW;
        if (arg.size() > 4 && arg.substr(0, 4) == "oro ")
        {
            try
            {
                cityCmd.goldAmount = static_cast<uint32_t>(std::stoul(arg.substr(4)));
                cityCmd.isGold = true;
            }
            catch (...)
            {
                sendChat(senderId, "Cantidad inválida.", ChatMsgType::INFO, monitor);
                return;
            }
        }
        else
        {
            cityCmd.itemName = arg;
            cityCmd.isGold = false;
        }
        auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
        sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
        if (result.ok)
        {
            sendStats(senderId, player, monitor);
            sendInventory(senderId, player, monitor);
        }
        return;
    }

    if (cmd == "lista")
    {
        if (!npcType)
        {
            sendChat(senderId, "No hay ningún NPC cerca.",
                     ChatMsgType::INFO, monitor);
            return;
        }

        CityCommand cityCmd;
        cityCmd.type = CityCommand::Type::LIST;

        auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
        sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
        return;
    }

    sendChat(senderId, "Comando desconocido: /" + cmd,
             ChatMsgType::INFO, monitor);
}

void ChatHandler::handleMeditate(uint32_t senderId,
                                 GameWorld &world,
                                 Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);
    if (!p.getCls().canUseMagic)
    {
        sendChat(senderId, "Tu clase no puede meditar.",
                 ChatMsgType::INFO, monitor);
        return;
    }
    if (p.isMeditating())
    {
        p.stopMeditating();
        sendChat(senderId, "Dejaste de meditar.", ChatMsgType::INFO, monitor);
    }
    else
    {
        p.startMeditating();
        sendChat(senderId, "Empezaste a meditar.", ChatMsgType::INFO, monitor);
    }
    sendStats(senderId, p, monitor);
}

void ChatHandler::handlePickItem(uint32_t senderId,
                                 GameWorld &world,
                                 Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);

    auto item = world.pickItemAt(p.getTileX(), p.getTileY());
    if (item)
    {
        if (p.getInventory().addItem(std::move(*item)))
        {
            sendInventory(senderId, p, monitor);
            sendChat(senderId, "Recogiste el objeto.", ChatMsgType::INFO, monitor);
        }
        else
        {
            sendChat(senderId, "Inventario lleno.", ChatMsgType::INFO, monitor);
        }
        return;
    }

    auto gold = world.pickGoldAt(p.getTileX(), p.getTileY());
    if (gold)
    {
        p.addGold(*gold);
        sendStats(senderId, p, monitor);
        sendChat(senderId,
                 "Recogiste " + std::to_string(*gold) + " oro.",
                 ChatMsgType::INFO, monitor);
        return;
    }

    sendChat(senderId, "No hay nada aquí.", ChatMsgType::INFO, monitor);
}

void ChatHandler::handleDropItem(uint32_t senderId,
                                 const std::string &itemName,
                                 GameWorld &world,
                                 Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);
    auto removed = p.getInventory().removeItemByName(itemName);
    if (!removed)
    {
        sendChat(senderId,
                 "No tenés '" + itemName + "' en el inventario.",
                 ChatMsgType::INFO, monitor);
        return;
    }
    world.addItemOnGround(std::move(*removed), p.getTileX(), p.getTileY());
    sendInventory(senderId, p, monitor);
    sendChat(senderId, "Tiraste " + itemName + " al suelo.",
             ChatMsgType::INFO, monitor);
}