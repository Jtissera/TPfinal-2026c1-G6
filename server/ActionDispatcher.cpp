#include "ActionDispatcher.h"

ActionDispatcher::ActionDispatcher() {
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MOVE)]       = &ActionDispatcher::handleMove;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_ATTACK)]     = &ActionDispatcher::handleAttack;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_PICK_ITEM)]  = &ActionDispatcher::handlePickItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_DROP_ITEM)]  = &ActionDispatcher::handleDropItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_EQUIP_ITEM)] = &ActionDispatcher::handleEquipItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MEDITATE)]   = &ActionDispatcher::handleMeditate;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT)]  = &ActionDispatcher::handleResurrect;
}

void ActionDispatcher::dispatch(const ClientMessage& msg, GameWorld& world, Monitor& monitor) {
    uint8_t opcode = msg.message->opCode();
    auto it = handlers.find(opcode);

    if (it != handlers.end())
        (this->*it->second)(msg.clientId, *msg.message, world, monitor);
    else
        std::cerr << "[Dispatcher] OpCode no manejado: 0x" << std::hex << (int)opcode << std::dec << std::endl;
}


void ActionDispatcher::sendStats(uint32_t id, Player& p, Monitor& monitor) {
    monitor.sendTo(id, std::make_shared<const PlayerStatsMessage>(
        p.getLevel(),
        p.getHp(), p.getMaxHp(),
        p.getMana(), p.getMaxMana(),
        p.getExp(), formulas.calcExpLimit(p.getLevel()),
        p.getGold()));
}

void ActionDispatcher::sendInventory(uint32_t id, Player& p, Monitor& monitor) {
    monitor.sendTo(id, std::make_shared<const InventoryUpdateMessage>(
        p.getInventory().getItems(),
        p.getInventory().getEquippedArray()));
}

void ActionDispatcher::sendDeath(uint32_t id, Player& dead, GameWorld& world, Monitor& monitor) {
    auto& inv = dead.getInventory();
    
    for (const Item& item : inv.getItems()) {
        auto removed = inv.removeItem(item.id);
        if (removed)
            world.addItemOnGround(std::move(*removed), dead.getX(), dead.getY());
    }

    monitor.sendTo(id, std::make_shared<const PlayerDiedMessage>(id));
    sendInventory(id, dead, monitor);
    sendStats(id, dead, monitor);
}


void ActionDispatcher::handleMove(uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor) {
    const auto& moveMsg = static_cast<const MoveMessage&>(msg);
    
    if (world.movePlayer(id, moveMsg.getDirection())) {
        monitor.sendTo(id, std::make_shared<const EntityMoveMessage>(
            (uint8_t)id, world.getX(id), world.getY(id)));
    }
}

void ActionDispatcher::handleAttack(uint32_t id, const Message& msg, 
                                     GameWorld& world, Monitor& monitor) {
    const auto& attackMsg = static_cast<const AttackMessage&>(msg);
    uint32_t targetId = attackMsg.getTargetId();

    Player& attacker = world.getPlayer(id);
    Player& target   = world.getPlayer(targetId);

    const Item* weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
    if (weapon && weapon->effect == ItemEffect::HEAL) {
        if (effects.apply(*weapon, attacker, &target)) {
            sendStats(id,       attacker, monitor);
            sendStats(targetId, target,   monitor);
        }
        return;
    }

    auto result = combat.attack(attacker, target);
    if (!result.valid) return;

    if (result.dodged) {
        sendStats(id,       attacker, monitor);
        sendStats(targetId, target,   monitor);
        return;
    }

    sendStats(id,       attacker, monitor);
    sendStats(targetId, target,   monitor);

    if (attacker.checkAndClearLevelUp())
        monitor.sendTo(id, std::make_shared<const LevelUpMessage>(attacker.getLevel()));

    if (result.killed) {
        world.handlePlayerDeath(targetId, id);

        monitor.sendTo(targetId, std::make_shared<const PlayerDiedMessage>(targetId));
        sendInventory(targetId, target, monitor);
        sendStats(targetId, target, monitor);
        sendStats(id, attacker, monitor);
    }
}

void ActionDispatcher::handleEquipItem(uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor) {
    const auto& equipMsg = static_cast<const EquipItemMessage&>(msg);
    Player& p = world.getPlayer(id);

    if (!p.isAlive()) return;

    if (p.getInventory().equipItem(equipMsg.getItemId()))
        sendInventory(id, p, monitor);
}

void ActionDispatcher::handleDropItem(uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor) {
    const auto& dropMsg = static_cast<const DropItemMessage&>(msg);
    
    Player& p = world.getPlayer(id);
    
    auto removed = p.getInventory().removeItem(dropMsg.getItemId());
    
    if (removed) {
        world.addItemOnGround(std::move(*removed), p.getX(), p.getY());
        sendInventory(id, p, monitor);
    }
}

void ActionDispatcher::handlePickItem(uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor) {
    Player& p = world.getPlayer(id);

    auto item = world.pickItemAt(p.getX(), p.getY());
    if (item && p.getInventory().addItem(std::move(*item)))
        sendInventory(id, p, monitor);

    auto gold = world.pickGoldAt(p.getX(), p.getY());
    if (gold) {
        p.addGold(*gold);
        sendStats(id, p, monitor);
    }
}

void ActionDispatcher::handleMeditate(uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor) {
    Player& p = world.getPlayer(id);
    if (!p.isAlive()) return;
    
    if (p.isMeditating())
        p.stopMeditating();
    else
        p.startMeditating();
    sendStats(id, p, monitor);
}

void ActionDispatcher::handleResurrect(uint32_t id, const Message& msg, GameWorld& world, Monitor& monitor) {
    Player& p = world.getPlayer(id);
    if (!p.isGhost()) return;

    // Spawn fija por ahora
    p.resurrect(6 * 96, 7 * 96);
    monitor.sendTo(id, std::make_shared<const EntityMoveMessage>((uint8_t)id, p.getX(), p.getY()));
    sendStats(id, p, monitor);
}