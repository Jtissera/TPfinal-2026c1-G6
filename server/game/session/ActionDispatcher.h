#pragma once
#include "../../clientMessage.h"
#include "../../monitorQueues.h"
#include "../../world/gameWorld.h"
#include "../combat/combatHandler.h"
#include "../combat/combatSystem.h"
#include "../combat/itemEffectHandler.h"
#include "../common/network/protocol/clientOpCode.h"
#include "../stats/gameFormulas.h"
#include <cstdint>
#include <iostream>
#include <unordered_map>

#include "common/network/messages/client/cheat/cheatMessage.h"
#include "common/network/messages/client/combat/attackMessage.h"
#include "common/network/messages/client/inventory/dropItemMessage.h"
#include "common/network/messages/client/inventory/equipItemMessage.h"
#include "common/network/messages/client/movement/moveMessage.h"
#include "common/network/messages/server/combat/combatLogMessage.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/player/EntityMoveMessage.h"
#include "common/network/messages/server/player/levelUpMessage.h"
#include "common/network/messages/server/player/playerDiedMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "server/game/chat/chatHandler.h"
#include "common/network/messages/client/chat/chatMessage.h"

#include "../../city/cityCommandParser.h"
#include "../../city/cityResult.h"
#include "../../../common/network/messages/client/city/interactNpcMessage.h"
#include "../../../common/network/messages/server/city/npcResponseMessage.h"
#include "../../../common/network/messages/server/error/errorMessage.h"

class ActionDispatcher
{
private:
  using ActionHandler = void (ActionDispatcher::*)(uint32_t, const Message &, GameWorld &, Monitor &);
  std::unordered_map<uint8_t, ActionHandler> handlers;

  CombatSystem combat;
  ItemEffectHandler effects;
  GameFormulas formulas;
  CombatHandler combatHandler;
  ChatHandler chatHandler;

  void handleMove(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);

  void handlePickItem(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);
  void handleDropItem(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);
  void handleEquipItem(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);
  void handleUnequipSlot(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);
  void handleUseItem(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);

  void handleMeditate(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);
  void handleResurrect(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);
  void handleCheat(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);

  void handleInteractNpc(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);

  void sendStats(uint32_t id, Player &p, Monitor &monitor);
  void sendInventory(uint32_t id, Player &p, Monitor &monitor);
  void sendDeath(uint32_t id, Player &dead, Monitor &monitor);

  void handleAttackPlayer(uint32_t attackerid, uint32_t targetId, GameWorld &world, Monitor &monitor);
  void handleAttackNpc(uint32_t attackerid, uint32_t targetId, GameWorld &world, Monitor &monitor);
  void sendLevelUpIfNeeded(uint32_t playerId, Player &player, Monitor &monitor);
  void handleAttack(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);
  void handleChat(uint32_t id, const Message &msg, GameWorld &world, Monitor &monitor);

public:
  explicit ActionDispatcher(const toml::table &config);
  void dispatch(const ClientMessage &msg, GameWorld &world, Monitor &monitor);
};