#pragma once

#include <cstdint>
#include <unordered_map>

#include <toml++/toml.hpp>

#include "server/clientMessage.h"
#include "server/monitorQueues.h"
#include "server/world/gameWorld.h"
#include "common/network/protocol/clientOpCode.h"
#include "server/game/stats/gameFormulas.h"
#include "handlers/cheatHandler.h"
#include "handlers/clanHandler.h"
#include "handlers/moveHandler.h"
#include "handlers/npcInteractionHandler.h"
#include "handlers/resurrectHandler.h"
#include "server/game/chat/chatHandler.h"
#include "handlers/combatHandler.h"
#include "handlers/inventoryHandler.h"
#include "common/network/messages/client/chat/chatMessage.h"

class ClanManager;

class ActionDispatcher
{
public:
  explicit ActionDispatcher(const toml::table &config,
                            ClanManager &clanManager);

  void dispatch(const ClientMessage &msg,
                GameWorld &world,
                Monitor &monitor);

private:
  using MemberHandler =
      void (ActionDispatcher::*)(uint32_t, const Message &,
                                 GameWorld &, Monitor &);

  std::unordered_map<uint8_t, MemberHandler> handlers;

  MoveHandler moveHandler;
  InventoryHandler inventoryHandler;
  CombatHandler combatHandler;
  ResurrectHandler resurrectHandler;
  CheatHandler cheatHandler;
  NpcInteractionHandler npcInteractionHandler;
  ClanHandler clanHandler;
  ChatHandler chatHandler;
  GameFormulas formulas;
  ClanManager &clanManager;

  void handleMove(uint32_t id, const Message &msg,
                  GameWorld &world, Monitor &monitor);
  void handlePickItem(uint32_t id, const Message &msg,
                      GameWorld &world, Monitor &monitor);
  void handleDropItem(uint32_t id, const Message &msg,
                      GameWorld &world, Monitor &monitor);
  void handleEquipItem(uint32_t id, const Message &msg,
                       GameWorld &world, Monitor &monitor);
  void handleUnequipSlot(uint32_t id, const Message &msg,
                         GameWorld &world, Monitor &monitor);
  void handleUseItem(uint32_t id, const Message &msg,
                     GameWorld &world, Monitor &monitor);
  void handleMeditate(uint32_t id, const Message &msg,
                      GameWorld &world, Monitor &monitor);
  void handleResurrect(uint32_t id, const Message &msg,
                       GameWorld &world, Monitor &monitor);
  void handleAttack(uint32_t id, const Message &msg,
                    GameWorld &world, Monitor &monitor);
  void handleCheat(uint32_t id, const Message &msg,
                   GameWorld &world, Monitor &monitor);
  void handleInteractNpc(uint32_t id, const Message &msg,
                         GameWorld &world, Monitor &monitor);
  void handleChat(uint32_t id, const Message &msg,
                  GameWorld &world, Monitor &monitor);
  void handleClanSync(uint32_t id, const Message &msg,
                      GameWorld &world, Monitor &monitor);
};