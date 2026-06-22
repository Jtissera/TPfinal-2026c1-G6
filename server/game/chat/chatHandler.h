#pragma once

#include "chatCommandParser.h"
#include "common/network/messages/server/chat/chatNotificationMessage.h"
#include "common/network/messages/server/inventory/itemPickedMessage.h"
#include "server/monitorQueues.h"
#include "server/world/gameWorld.h"
#include "server/game/player/Player.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/inventory/itemOnGroundMessage.h"
#include "common/network/messages/server/player/playerEquipmentUpdateMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "server/game/stats/gameFormulas.h"
#include "server/city/cityCommandParser.h"
#include "server/city/cityResult.h"
#include "server/game/equipmentDtoFactory.h"
#include "server/game/clan/clanManager.h"
#include "server/game/clan/clan.h"

class ChatHandler
{
public:
    explicit ChatHandler(ClanManager &clanManager, const toml::table &config);

    bool handle(uint32_t senderId,
                const std::string &rawText,
                uint32_t targetId,
                GameWorld &world,
                Monitor &monitor);

private:
    ClanManager &clanManager;
    GameFormulas formulas;
    uint8_t clanMinLevelToFound;

    void sendChat(uint32_t clientId,
                  const std::string &text,
                  ChatMsgType type,
                  Monitor &monitor);

    void broadcastChat(const std::string &text,
                       ChatMsgType type,
                       Monitor &monitor);

    void sendStats(uint32_t id, Player &p, Monitor &monitor);
    void sendInventory(uint32_t id, Player &p, Monitor &monitor);

    void handleGeneral(uint32_t senderId,
                       const std::string &text,
                       GameWorld &world,
                       Monitor &monitor);

    void handlePrivate(uint32_t senderId,
                       const std::string &targetNick,
                       const std::string &text,
                       GameWorld &world,
                       Monitor &monitor);

    void handleCommand(uint32_t senderId,
                       const ParsedChatInput &parsed,
                       uint32_t targetId,
                       GameWorld &world,
                       Monitor &monitor);

    void handleMeditate(uint32_t senderId, GameWorld &world, Monitor &monitor);
    void handlePickItem(uint32_t senderId, GameWorld &world, Monitor &monitor);
    void handleDropItem(uint32_t senderId,
                        const std::string &itemName,
                        GameWorld &world,
                        Monitor &monitor);
    void handleResurrect(uint32_t senderId, GameWorld &world, Monitor &monitor);

    void handleCityCommands(uint32_t senderId,
                            const ParsedChatInput &parsed,
                            GameWorld &world,
                            Monitor &monitor);

    void handleHeal(uint32_t senderId,
                    std::optional<NpcType> npcType,
                    GameWorld &world,
                    Monitor &monitor);

    void handleBuy(uint32_t senderId,
                   const std::string &arg,
                   std::optional<NpcType> npcType,
                   GameWorld &world,
                   Monitor &monitor);

    void handleSell(uint32_t senderId,
                    const std::string &arg,
                    std::optional<NpcType> npcType,
                    GameWorld &world,
                    Monitor &monitor);

    void handleDeposit(uint32_t senderId,
                       const std::string &arg,
                       std::optional<NpcType> npcType,
                       GameWorld &world,
                       Monitor &monitor);

    void handleWithdraw(uint32_t senderId,
                        const std::string &arg,
                        std::optional<NpcType> npcType,
                        GameWorld &world,
                        Monitor &monitor);

    void handleList(uint32_t senderId,
                    std::optional<NpcType> npcType,
                    GameWorld &world,
                    Monitor &monitor);

    void handleClanCommands(uint32_t senderId,
                            const ParsedChatInput &parsed,
                            GameWorld &world,
                            Monitor &monitor);

    void handleFoundClan(uint32_t senderId,
                         const std::string &clanName,
                         GameWorld &world,
                         Monitor &monitor);

    void handleJoinClan(uint32_t senderId,
                        const std::string &clanName,
                        GameWorld &world,
                        Monitor &monitor);

    void handleLeaveClan(uint32_t senderId,
                         GameWorld &world,
                         Monitor &monitor);

    void handleReviewClan(uint32_t senderId,
                          GameWorld &world,
                          Monitor &monitor);

    void handleClanMemberAction(uint32_t senderId,
                                const std::string &cmd,
                                const std::string &targetNick,
                                GameWorld &world,
                                Monitor &monitor);

    std::optional<NpcType> findAdjacentNpc(const Player &player,
                                           GameWorld &world) const;

    CityCommand buildGoldOrItemCommand(CityCommand::Type type,
                                       const std::string &arg) const;
};