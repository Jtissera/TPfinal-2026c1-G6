#pragma once
#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <cstdlib>
#include "world/gameWorld.h"
#include "monitorQueues.h"
#include "clientMessage.h"
#include "game/combatSystem.h"
#include "game/itemEffectHandler.h"
#include "game/gameFormulas.h"
#include "../common/network/protocol/clientOpCode.h"

#include "common/network/messages/client/movement/moveMessage.h"
#include "common/network/messages/client/combat/attackMessage.h"
#include "common/network/messages/client/inventory/equipItemMessage.h"
#include "common/network/messages/client/inventory/dropItemMessage.h"
#include "common/network/messages/server/player/EntityMoveMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "common/network/messages/server/player/playerDiedMessage.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/combat/combatLogMessage.h"
#include "common/network/messages/server/player/levelUpMessage.h"

class ActionDispatcher {
private:
    using ActionHandler = void (ActionDispatcher::*)(uint32_t, const Message&, GameWorld&, Monitor&);
    std::unordered_map<uint8_t, ActionHandler> handlers;

    CombatSystem    combat;
    ItemEffectHandler effects;
    GameFormulas    formulas;

    void handleMove     (uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor);
    void handleAttack   (uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor);
    void handlePickItem (uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor);
    void handleDropItem (uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor);
    void handleEquipItem(uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor);
    void handleMeditate (uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor);
    void handleResurrect(uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor);

    void sendStats    (uint32_t id, Player& p, Monitor& monitor);
    void sendInventory(uint32_t id, Player& p, Monitor& monitor);
    void sendDeath    (uint32_t id, Player& dead, GameWorld& world, Monitor& monitor);

public:
    ActionDispatcher();
    void dispatch(const ClientMessage& msg, GameWorld& world, Monitor& monitor);
};