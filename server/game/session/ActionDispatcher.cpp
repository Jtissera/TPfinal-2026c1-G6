#include "ActionDispatcher.h"

#include <iostream>

ActionDispatcher::ActionDispatcher(const toml::table &config,
                                   ClanManager &clanManager)
    : moveHandler(), inventoryHandler(config), combatHandler(config, clanManager), resurrectHandler(config),
      cheatHandler(config), npcInteractionHandler(config), clanHandler(), chatHandler(clanManager, config),
      formulas(config), clanManager(clanManager)
{
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MOVE)] =
        &ActionDispatcher::handleMove;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_PICK_ITEM)] =
        &ActionDispatcher::handlePickItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_DROP_ITEM)] =
        &ActionDispatcher::handleDropItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_EQUIP_ITEM)] =
        &ActionDispatcher::handleEquipItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_UNEQUIP_SLOT)] =
        &ActionDispatcher::handleUnequipSlot;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_USE_ITEM)] =
        &ActionDispatcher::handleUseItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MEDITATE)] =
        &ActionDispatcher::handleMeditate;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT)] =
        &ActionDispatcher::handleResurrect;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_ATTACK)] =
        &ActionDispatcher::handleAttack;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_CHEAT)] =
        &ActionDispatcher::handleCheat;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_INTERACT_NPC)] =
        &ActionDispatcher::handleInteractNpc;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_CHAT)] =
        &ActionDispatcher::handleChat;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_CLAN_SYNC_INTERNAL)] =
        &ActionDispatcher::handleClanSync;
}

void ActionDispatcher::dispatch(const ClientMessage &msg,
                                GameWorld &world,
                                Monitor &monitor)
{
    const uint8_t opcode = msg.message->opCode();

    if (!world.canPlayerAct(msg.clientId))
    {
        const bool allowed =
            opcode == static_cast<uint8_t>(ClientOpCode::MSG_MOVE) ||
            opcode == static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT) ||
            opcode == static_cast<uint8_t>(ClientOpCode::MSG_INTERACT_NPC) ||
            opcode == static_cast<uint8_t>(ClientOpCode::MSG_CHAT) ||
            opcode == static_cast<uint8_t>(
                          ClientOpCode::MSG_CLAN_SYNC_INTERNAL);
        if (!allowed)
        {
            return;
        }
    }

    std::unordered_map<uint8_t, MemberHandler>::iterator it =
        handlers.find(opcode);
    if (it != handlers.end())
    {
        (this->*it->second)(msg.clientId, *msg.message, world, monitor);
    }
    else
    {
        std::cerr << "[Dispatcher] Unhandled opcode: 0x"
                  << std::hex << static_cast<int>(opcode)
                  << std::dec << std::endl;
    }
}

void ActionDispatcher::handleMove(uint32_t id, const Message &msg,
                                  GameWorld &world, Monitor &monitor)
{
    moveHandler.handle(id, msg, world, monitor);
}

void ActionDispatcher::handlePickItem(uint32_t id, const Message &msg,
                                      GameWorld &world, Monitor &monitor)
{
    inventoryHandler.handlePickItem(id, msg, world, monitor);
}

void ActionDispatcher::handleDropItem(uint32_t id, const Message &msg,
                                      GameWorld &world, Monitor &monitor)
{
    inventoryHandler.handleDropItem(id, msg, world, monitor);
}

void ActionDispatcher::handleEquipItem(uint32_t id, const Message &msg,
                                       GameWorld &world, Monitor &monitor)
{
    inventoryHandler.handleEquipItem(id, msg, world, monitor);
}

void ActionDispatcher::handleUnequipSlot(uint32_t id, const Message &msg,
                                         GameWorld &world, Monitor &monitor)
{
    inventoryHandler.handleUnequipSlot(id, msg, world, monitor);
}

void ActionDispatcher::handleUseItem(uint32_t id, const Message &msg,
                                     GameWorld &world, Monitor &monitor)
{
    inventoryHandler.handleUseItem(id, msg, world, monitor);
}

void ActionDispatcher::handleMeditate(uint32_t id, const Message &msg,
                                      GameWorld &world, Monitor &monitor)
{
    resurrectHandler.handleMeditate(id, msg, world, monitor);
}

void ActionDispatcher::handleResurrect(uint32_t id, const Message &msg,
                                       GameWorld &world, Monitor &monitor)
{
    resurrectHandler.handleResurrect(id, msg, world, monitor);
}

void ActionDispatcher::handleAttack(uint32_t id, const Message &msg,
                                    GameWorld &world, Monitor &monitor)
{
    combatHandler.handle(id, msg, world, monitor);
}

void ActionDispatcher::handleCheat(uint32_t id, const Message &msg,
                                   GameWorld &world, Monitor &monitor)
{
    cheatHandler.handle(id, msg, world, monitor);
}

void ActionDispatcher::handleInteractNpc(uint32_t id, const Message &msg,
                                         GameWorld &world, Monitor &monitor)
{
    npcInteractionHandler.handle(id, msg, world, monitor);
}

void ActionDispatcher::handleChat(uint32_t id, const Message &msg,
                                  GameWorld &world, Monitor &monitor)
{
    const ChatMessage &chatMsg = static_cast<const ChatMessage &>(msg);
    chatHandler.handle(id,
                       chatMsg.getText(),
                       chatMsg.getTargetId(),
                       world,
                       monitor);
}

void ActionDispatcher::handleClanSync(uint32_t id, const Message &msg,
                                      GameWorld &world, Monitor &monitor)
{
    clanHandler.handle(id, msg, world, monitor);
}