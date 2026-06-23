#pragma once

#include <cstdint>
#include <string>

#include <toml++/toml.hpp>

#include "common/network/messages/message.h"
#include "server/monitorQueues.h"
#include "server/world/gameWorld.h"
#include "../../combat/combatSystem.h"
#include "../../combat/itemEffectHandler.h"
#include "server/game/stats/gameFormulas.h"
#include "common/network/messages/client/combat/attackMessage.h"
#include "common/network/messages/server/chat/chatNotificationMessage.h"
#include "common/network/messages/server/combat/combatLogMessage.h"
#include "common/network/messages/server/player/playerAttackVisualMessage.h"
#include "common/network/messages/server/inventory/goldOnGroundMessage.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/inventory/itemOnGroundMessage.h"
#include "common/network/messages/server/npc/npcHealthMessage.h"
#include "common/network/messages/server/player/levelUpMessage.h"
#include "common/network/messages/server/player/playerDiedMessage.h"
#include "common/network/messages/server/player/playerHealthMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "server/game/clan/clanManager.h"
#include "server/game/combat/combatResolver.h"
#include "../../../world/DeathResult.h"

class CombatHandler
{
public:
    explicit CombatHandler(const toml::table &config, ClanManager &clanManager);

    void handle(uint32_t clientId,
                const Message &msg,
                GameWorld &world,
                Monitor &monitor);

private:
    GameFormulas formulas;
    CombatSystem combatSystem;
    ItemEffectHandler itemEffectHandler;
    CombatResolver resolver;
    ClanManager &clanManager;

    void handleAttackPlayer(uint32_t attackerId,
                            uint32_t targetId,
                            GameWorld &world,
                            Monitor &monitor);

    void handleAttackNpc(uint32_t attackerId,
                         uint32_t npcId,
                         GameWorld &world,
                         Monitor &monitor);

    void sendStats(uint32_t clientId, Player &player, Monitor &monitor);
    void sendInventory(uint32_t clientId, Player &player, Monitor &monitor);
    void sendLevelUpIfNeeded(uint32_t clientId, Player &player, Monitor &monitor);

    PlayerAttackVisualType resolveAttackVisualType(const Player &attacker) const;
    void broadcastPlayerAttackVisual(uint32_t attackerId,
                                     uint32_t targetId,
                                     const Player &attacker,
                                     Monitor &monitor);
    std::string resolveAttackEffectId(const Player &attacker) const;
};