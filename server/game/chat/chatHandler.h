#pragma once

#include "chatCommandParser.h"
#include "common/network/messages/server/chat/chatNotificationMessage.h"
#include "server/monitorQueues.h"
#include "server/world/gameWorld.h"
#include "server/game/player/Player.h"

class ChatHandler
{
public:
    bool handle(uint32_t senderId,
                const std::string &rawText,
                uint32_t targetId,
                GameWorld &world,
                Monitor &monitor);

private:
    static void sendChat(uint32_t clientId,
                         const std::string &text,
                         ChatMsgType type,
                         Monitor &monitor);

    static void broadcastChat(const std::string &text,
                              ChatMsgType type,
                              Monitor &monitor);

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
    void handleDropItem(uint32_t senderId, const std::string &itemName,
                        GameWorld &world, Monitor &monitor);

    static void sendStats(uint32_t id, Player &p, Monitor &monitor);
    static void sendInventory(uint32_t id, Player &p, Monitor &monitor);
};
