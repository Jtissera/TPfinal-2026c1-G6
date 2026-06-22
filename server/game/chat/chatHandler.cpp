#include "chatHandler.h"

ChatHandler::ChatHandler(ClanManager &clanManager, const toml::table &config)
    : clanManager(clanManager),
      formulas(config),
      clanMinLevelToFound(config["clan"]["min_level_to_found"].value_or<uint8_t>(6))
{
}

void ChatHandler::sendChat(uint32_t clientId, const std::string &text,
                           ChatMsgType type, Monitor &monitor)
{
    monitor.sendTo(clientId, std::make_shared<const ChatNotificationMessage>(text, type));
}

void ChatHandler::broadcastChat(const std::string &text,
                                ChatMsgType type, Monitor &monitor)
{
    monitor.broadcast(std::make_shared<const ChatNotificationMessage>(text, type));
}

void ChatHandler::sendStats(uint32_t id, Player &p, Monitor &monitor)
{
    monitor.sendTo(id, std::make_shared<const PlayerStatsMessage>(
                           p.getLevel(), p.getHp(), p.getMaxHp(),
                           p.getMana(), p.getMaxMana(),
                           p.getExp(), formulas.calcExpLimit(p.getLevel()),
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
        handlePrivate(senderId, parsed->targetNick, parsed->keyword, world, monitor);
        break;
    case ChatInputType::COMMAND:
        handleCommand(senderId, *parsed, targetId, world, monitor);
        break;
    }
    return true;
}

void ChatHandler::handleGeneral(uint32_t senderId, const std::string &text,
                                GameWorld &world, Monitor &monitor)
{
    const Player &sender = world.getPlayer(senderId);
    broadcastChat("[" + sender.getName() + "]: " + text, ChatMsgType::GENERAL, monitor);
}

void ChatHandler::handlePrivate(uint32_t senderId, const std::string &targetNick,
                                const std::string &text,
                                GameWorld &world, Monitor &monitor)
{
    const Player &sender = world.getPlayer(senderId);

    std::optional<uint32_t> targetId = world.findPlayerIdByName(targetNick);
    if (!targetId)
    {
        sendChat(senderId, "El jugador '" + targetNick + "' no está conectado.",
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
                                uint32_t /*targetId*/,
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
            sendChat(senderId, "Uso: /tirar <nombre_item>", ChatMsgType::INFO, monitor);
            return;
        }
        handleDropItem(senderId, arg, world, monitor);
        return;
    }
    if (cmd == "resucitar")
    {
        handleResurrect(senderId, world, monitor);
        return;
    }

    if (cmd == "fundar-clan" || cmd == "unirse" || cmd == "dejar-clan" ||
        cmd == "revisar-clan" || cmd == "clan-aceptar" || cmd == "clan-rechazar" ||
        cmd == "clan-ban" || cmd == "clan-kick")
    {
        handleClanCommands(senderId, parsed, world, monitor);
        return;
    }

    if (cmd == "curar" || cmd == "comprar" || cmd == "vender" ||
        cmd == "depositar" || cmd == "retirar" || cmd == "lista")
    {
        handleCityCommands(senderId, parsed, world, monitor);
        return;
    }

    sendChat(senderId, "Comando desconocido: /" + cmd, ChatMsgType::INFO, monitor);
}

void ChatHandler::handleResurrect(uint32_t senderId,
                                  GameWorld &world, Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);
    if (!p.isGhost())
    {
        sendChat(senderId, "No estás muerto.", ChatMsgType::INFO, monitor);
        return;
    }
    auto result = world.handleRemoteResurrect(senderId);
    sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
}

void ChatHandler::handleMeditate(uint32_t senderId,
                                 GameWorld &world, Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);
    if (!p.getCls().canUseMagic)
    {
        sendChat(senderId, "Tu clase no puede meditar.", ChatMsgType::INFO, monitor);
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
                                 GameWorld &world, Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);

    if (p.isGhost())
    {
        sendChat(senderId, "Los muertos no pueden tomar objetos.", ChatMsgType::INFO, monitor);
        return;
    }

    for (const GroundItem &groundItem : world.getGroundManager().getAllItems())
    {
        if (groundItem.tileX != p.getTileX() || groundItem.tileY != p.getTileY())
            continue;

        if (!p.getInventory().canAddItem())
        {
            sendChat(senderId, "Inventario lleno.", ChatMsgType::INFO, monitor);
            return;
        }

        uint32_t instanceId = groundItem.item.instanceId;
        auto item = world.pickItemById(instanceId);

        if (!item)
        {
            sendChat(senderId, "El objeto ya no está disponible.", ChatMsgType::INFO, monitor);
            return;
        }

        if (p.getInventory().addItem(std::move(*item)))
        {
            sendInventory(senderId, p, monitor);
            monitor.broadcast(std::make_shared<const ItemPickedMessage>(senderId, instanceId));
            sendChat(senderId, "Recogiste el objeto.", ChatMsgType::INFO, monitor);
        }
        else
        {
            sendChat(senderId, "Inventario lleno.", ChatMsgType::INFO, monitor);
        }
        return;
    }

    for (const GroundGold &groundGold : world.getGroundManager().getAllGold())
    {
        if (groundGold.tileX != p.getTileX() || groundGold.tileY != p.getTileY())
            continue;

        uint32_t instanceId = groundGold.instanceId;
        auto gold = world.pickGoldById(instanceId);

        if (!gold)
        {
            sendChat(senderId, "El oro ya no está disponible.", ChatMsgType::INFO, monitor);
            return;
        }

        p.addGold(*gold);
        sendStats(senderId, p, monitor);
        monitor.broadcast(std::make_shared<const ItemPickedMessage>(senderId, instanceId));
        sendChat(senderId, "Recogiste " + std::to_string(*gold) + " oro.",
                 ChatMsgType::INFO, monitor);
        return;
    }

    sendChat(senderId, "No hay nada aquí.", ChatMsgType::INFO, monitor);
}

void ChatHandler::handleDropItem(uint32_t senderId, const std::string &itemName,
                                 GameWorld &world, Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);
    auto removed = p.getInventory().removeItemByName(itemName);
    if (!removed)
    {
        sendChat(senderId, "No tenés '" + itemName + "' en el inventario.",
                 ChatMsgType::INFO, monitor);
        return;
    }

    const int tileX = p.getTileX();
    const int tileY = p.getTileY();
    Item itemForMessage = *removed;

    world.addItemOnGround(std::move(*removed), tileX, tileY);
    sendInventory(senderId, p, monitor);

    monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(
        senderId, buildEquipmentDtoFromPlayer(p)));

    monitor.broadcast(std::make_shared<const ItemOnGroundMessage>(
        itemForMessage, tileX, tileY));

    sendChat(senderId, "Tiraste " + itemName + " al suelo.", ChatMsgType::INFO, monitor);
}

std::optional<NpcType> ChatHandler::findAdjacentNpc(const Player &player,
                                                    GameWorld &world) const
{
    const int px = player.getTileX();
    const int py = player.getTileY();

    std::optional<NpcType> npcType;
    for (int dx = -1; dx <= 1 && !npcType; dx++)
        for (int dy = -1; dy <= 1 && !npcType; dy++)
            npcType = world.getNpcTypeAtTile(px + dx, py + dy);

    return npcType;
}

CityCommand ChatHandler::buildGoldOrItemCommand(CityCommand::Type type,
                                                const std::string &arg) const
{
    CityCommand cityCmd;
    cityCmd.type = type;

    if (arg.size() > 4 && arg.substr(0, 4) == "oro ")
    {
        cityCmd.goldAmount = static_cast<uint32_t>(std::stoul(arg.substr(4)));
        cityCmd.isGold = true;
    }
    else
    {
        cityCmd.itemName = arg;
        cityCmd.isGold = false;
    }
    return cityCmd;
}

void ChatHandler::handleCityCommands(uint32_t senderId,
                                     const ParsedChatInput &parsed,
                                     GameWorld &world, Monitor &monitor)
{
    const std::string &cmd = parsed.keyword;
    const std::string &arg = parsed.argument;

    Player &player = world.getPlayer(senderId);
    auto npcType = findAdjacentNpc(player, world);

    if (cmd == "curar")
    {
        handleHeal(senderId, npcType, world, monitor);
        return;
    }
    if (cmd == "comprar")
    {
        if (arg.empty())
        {
            sendChat(senderId, "Uso: /comprar <nombre_item>", ChatMsgType::INFO, monitor);
            return;
        }
        handleBuy(senderId, arg, npcType, world, monitor);
        return;
    }
    if (cmd == "vender")
    {
        if (arg.empty())
        {
            sendChat(senderId, "Uso: /vender <nombre_item>", ChatMsgType::INFO, monitor);
            return;
        }
        handleSell(senderId, arg, npcType, world, monitor);
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
        handleDeposit(senderId, arg, npcType, world, monitor);
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
        handleWithdraw(senderId, arg, npcType, world, monitor);
        return;
    }
    if (cmd == "lista")
    {
        handleList(senderId, npcType, world, monitor);
        return;
    }
}

void ChatHandler::handleHeal(uint32_t senderId, std::optional<NpcType> npcType,
                             GameWorld &world, Monitor &monitor)
{
    if (!npcType || *npcType != NpcType::PRIEST)
    {
        sendChat(senderId, "No hay ningún sacerdote cerca.", ChatMsgType::INFO, monitor);
        return;
    }
    CityCommand cityCmd;
    cityCmd.type = CityCommand::Type::HEAL;
    auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
    sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
    if (result.ok)
    {
        Player &player = world.getPlayer(senderId);
        sendStats(senderId, player, monitor);
    }
}

void ChatHandler::handleBuy(uint32_t senderId, const std::string &arg,
                            std::optional<NpcType> npcType,
                            GameWorld &world, Monitor &monitor)
{
    if (!npcType)
    {
        sendChat(senderId, "No hay ningún NPC cerca.", ChatMsgType::INFO, monitor);
        return;
    }
    CityCommand cityCmd;
    cityCmd.type = CityCommand::Type::BUY;
    cityCmd.itemName = arg;
    auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
    sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
    if (result.ok)
    {
        Player &player = world.getPlayer(senderId);
        sendStats(senderId, player, monitor);
        sendInventory(senderId, player, monitor);
    }
}

void ChatHandler::handleSell(uint32_t senderId, const std::string &arg,
                             std::optional<NpcType> npcType,
                             GameWorld &world, Monitor &monitor)
{
    if (!npcType || *npcType != NpcType::MERCHANT)
    {
        sendChat(senderId, "No hay ningún comerciante cerca.", ChatMsgType::INFO, monitor);
        return;
    }
    CityCommand cityCmd;
    cityCmd.type = CityCommand::Type::SELL;
    cityCmd.itemName = arg;
    auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
    sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
    if (result.ok)
    {
        Player &player = world.getPlayer(senderId);
        sendStats(senderId, player, monitor);
        sendInventory(senderId, player, monitor);
    }
}

void ChatHandler::handleDeposit(uint32_t senderId, const std::string &arg,
                                std::optional<NpcType> npcType,
                                GameWorld &world, Monitor &monitor)
{
    if (!npcType || *npcType != NpcType::BANKER)
    {
        sendChat(senderId, "No hay ningún banquero cerca.", ChatMsgType::INFO, monitor);
        return;
    }
    try
    {
        CityCommand cityCmd = buildGoldOrItemCommand(CityCommand::Type::DEPOSIT, arg);
        auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
        sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
        if (result.ok)
        {
            Player &player = world.getPlayer(senderId);
            sendStats(senderId, player, monitor);
            sendInventory(senderId, player, monitor);
        }
    }
    catch (...)
    {
        sendChat(senderId, "Cantidad inválida.", ChatMsgType::INFO, monitor);
    }
}

void ChatHandler::handleWithdraw(uint32_t senderId, const std::string &arg,
                                 std::optional<NpcType> npcType,
                                 GameWorld &world, Monitor &monitor)
{
    if (!npcType || *npcType != NpcType::BANKER)
    {
        sendChat(senderId, "No hay ningún banquero cerca.", ChatMsgType::INFO, monitor);
        return;
    }
    try
    {
        CityCommand cityCmd = buildGoldOrItemCommand(CityCommand::Type::WITHDRAW, arg);
        auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
        sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
        if (result.ok)
        {
            Player &player = world.getPlayer(senderId);
            sendStats(senderId, player, monitor);
            sendInventory(senderId, player, monitor);
        }
    }
    catch (...)
    {
        sendChat(senderId, "Cantidad inválida.", ChatMsgType::INFO, monitor);
    }
}

void ChatHandler::handleList(uint32_t senderId, std::optional<NpcType> npcType,
                             GameWorld &world, Monitor &monitor)
{
    if (!npcType)
    {
        sendChat(senderId, "No hay ningún NPC cerca.", ChatMsgType::INFO, monitor);
        return;
    }
    CityCommand cityCmd;
    cityCmd.type = CityCommand::Type::LIST;
    auto result = world.handleCityInteraction(senderId, *npcType, cityCmd);
    sendChat(senderId, result.message, ChatMsgType::INFO, monitor);
}

void ChatHandler::handleClanCommands(uint32_t senderId,
                                     const ParsedChatInput &parsed,
                                     GameWorld &world, Monitor &monitor)
{
    const std::string &cmd = parsed.keyword;
    const std::string &arg = parsed.argument;

    if (cmd == "fundar-clan")
    {
        if (arg.empty())
        {
            sendChat(senderId, "Uso: /fundar-clan <nombre>", ChatMsgType::INFO, monitor);
            return;
        }
        handleFoundClan(senderId, arg, world, monitor);
        return;
    }
    if (cmd == "unirse")
    {
        if (arg.empty())
        {
            sendChat(senderId, "Uso: /unirse <nombre del clan>", ChatMsgType::INFO, monitor);
            return;
        }
        handleJoinClan(senderId, arg, world, monitor);
        return;
    }
    if (cmd == "dejar-clan")
    {
        handleLeaveClan(senderId, world, monitor);
        return;
    }
    if (cmd == "revisar-clan")
    {
        handleReviewClan(senderId, world, monitor);
        return;
    }

    // clan-aceptar / clan-rechazar / clan-ban / clan-kick
    if (arg.empty())
    {
        sendChat(senderId, "Uso: /" + cmd + " <nick>", ChatMsgType::INFO, monitor);
        return;
    }
    handleClanMemberAction(senderId, cmd, arg, world, monitor);
}

void ChatHandler::handleFoundClan(uint32_t senderId, const std::string &clanName,
                                  GameWorld &world, Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);

    if (p.getLevel() < clanMinLevelToFound)
    {
        sendChat(senderId,
                 "Necesitás ser nivel " + std::to_string(clanMinLevelToFound) +
                     " o más para fundar un clan.",
                 ChatMsgType::INFO, monitor);
        return;
    }

    auto result = clanManager.foundClan(clanName, p.getName());

    switch (result)
    {
    case ClanManager::Result::NAME_TAKEN:
        sendChat(senderId, "Ya existe un clan llamado '" + clanName + "'.",
                 ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::ALREADY_IN_CLAN:
        sendChat(senderId, "Ya pertenecés a un clan.", ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::OK:
        clanManager.syncPlayerClanState(p.getName());
        sendChat(senderId, "¡Fundaste el clan '" + clanName + "'!",
                 ChatMsgType::CLAN, monitor);
        return;
    default:
        sendChat(senderId, "No se pudo fundar el clan.", ChatMsgType::INFO, monitor);
        return;
    }
}

void ChatHandler::handleJoinClan(uint32_t senderId, const std::string &clanName,
                                 GameWorld &world, Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);
    auto result = clanManager.applyToJoin(clanName, p.getName());

    switch (result)
    {
    case ClanManager::Result::CLAN_NOT_FOUND:
        sendChat(senderId, "No existe ningún clan llamado '" + clanName + "'.",
                 ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::ALREADY_IN_CLAN:
        sendChat(senderId, "Ya pertenecés a un clan. Usá /dejar-clan primero.",
                 ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::BANNED:
        sendChat(senderId, "No podés unirte a ese clan.", ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::ALREADY_APPLIED:
        sendChat(senderId, "Ya tenés una solicitud pendiente para ese clan.",
                 ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::OK:
        sendChat(senderId, "Solicitud enviada al clan '" + clanName + "'.",
                 ChatMsgType::CLAN, monitor);
        return;
    default:
        sendChat(senderId, "No se pudo procesar la solicitud.", ChatMsgType::INFO, monitor);
        return;
    }
}

void ChatHandler::handleLeaveClan(uint32_t senderId,
                                  GameWorld &world, Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);

    auto clanInfo = clanManager.findClanInfoForMember(p.getName());
    std::string clanName = clanInfo ? clanInfo->first : "";

    auto result = clanManager.leaveClan(p.getName());

    switch (result)
    {
    case ClanManager::Result::NOT_A_MEMBER:
        sendChat(senderId, "No pertenecés a ningún clan.", ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::FOUNDER_CANNOT_LEAVE:
        sendChat(senderId, "El fundador no puede abandonar su propio clan.",
                 ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::OK:
        sendChat(senderId, "Dejaste el clan '" + clanName + "'.",
                 ChatMsgType::CLAN, monitor);
        clanManager.syncPlayerClanState(p.getName());
        return;
    default:
        sendChat(senderId, "No se pudo procesar la salida del clan.",
                 ChatMsgType::INFO, monitor);
        return;
    }
}

void ChatHandler::handleReviewClan(uint32_t senderId,
                                   GameWorld &world, Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);
    auto clanInfo = clanManager.findClanInfoForMember(p.getName());

    if (!clanInfo || !clanInfo->second)
    {
        sendChat(senderId, "Solo el fundador de un clan puede revisarlo.",
                 ChatMsgType::INFO, monitor);
        return;
    }

    auto overview = clanManager.getOverviewForFounder(p.getName());
    if (!overview)
    {
        sendChat(senderId, "No se encontró información de tu clan.",
                 ChatMsgType::INFO, monitor);
        return;
    }

    std::string info = "Clan '" + overview->clanName + "' - Miembros (" +
                       std::to_string(overview->members.size()) + "/" +
                       std::to_string(Clan::MAX_MEMBERS) + "): ";
    for (size_t i = 0; i < overview->members.size(); ++i)
    {
        if (i > 0)
            info += ", ";
        info += overview->members[i];
    }
    sendChat(senderId, info, ChatMsgType::CLAN, monitor);

    if (overview->applicants.empty())
    {
        sendChat(senderId, "No hay solicitudes pendientes.", ChatMsgType::CLAN, monitor);
        return;
    }

    std::string pending = "Solicitudes pendientes: ";
    for (size_t i = 0; i < overview->applicants.size(); ++i)
    {
        if (i > 0)
            pending += ", ";
        pending += overview->applicants[i];
    }
    sendChat(senderId, pending, ChatMsgType::CLAN, monitor);
}

void ChatHandler::handleClanMemberAction(uint32_t senderId,
                                         const std::string &cmd,
                                         const std::string &targetNick,
                                         GameWorld &world, Monitor &monitor)
{
    Player &p = world.getPlayer(senderId);
    auto clanInfo = clanManager.findClanInfoForMember(p.getName());

    if (!clanInfo || !clanInfo->second)
    {
        sendChat(senderId, "Solo el fundador de un clan puede usar ese comando.",
                 ChatMsgType::INFO, monitor);
        return;
    }

    const std::string &clanName = clanInfo->first;
    ClanManager::Result result;
    std::string successMsgToSender;
    std::string successMsgToTarget;

    if (cmd == "clan-aceptar")
    {
        result = clanManager.acceptApplicant(p.getName(), targetNick);
        successMsgToSender = targetNick + " fue aceptado en el clan.";
        successMsgToTarget = "¡Fuiste aceptado en el clan '" + clanName + "'!";
    }
    else if (cmd == "clan-rechazar")
    {
        result = clanManager.rejectApplicant(p.getName(), targetNick);
        successMsgToSender = "Rechazaste la solicitud de " + targetNick + ".";
        successMsgToTarget = "Tu solicitud al clan '" + clanName + "' fue rechazada.";
    }
    else if (cmd == "clan-ban")
    {
        result = clanManager.banPlayer(p.getName(), targetNick);
        successMsgToSender = targetNick + " fue baneado del clan.";
        successMsgToTarget = "Fuiste baneado del clan '" + clanName + "'.";
    }
    else // clan-kick
    {
        result = clanManager.kickMember(p.getName(), targetNick);
        successMsgToSender = targetNick + " fue expulsado del clan.";
        successMsgToTarget = "Fuiste expulsado del clan '" + clanName + "'.";
    }

    switch (result)
    {
    case ClanManager::Result::NOT_AN_APPLICANT:
        sendChat(senderId, targetNick + " no tiene una solicitud pendiente.",
                 ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::NOT_A_MEMBER:
        sendChat(senderId, targetNick + " no pertenece a tu clan.",
                 ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::CLAN_FULL:
        sendChat(senderId, "Tu clan está lleno.", ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::CANNOT_KICK_SELF:
        sendChat(senderId, "No podés expulsarte a vos mismo. Usá /dejar-clan.",
                 ChatMsgType::INFO, monitor);
        return;
    case ClanManager::Result::OK:
        sendChat(senderId, successMsgToSender, ChatMsgType::CLAN, monitor);
        clanManager.syncPlayerClanState(targetNick);
        clanManager.notifyPlayer(targetNick, successMsgToTarget);
        return;
    default:
        sendChat(senderId, "No se pudo procesar el comando.", ChatMsgType::INFO, monitor);
        return;
    }
}