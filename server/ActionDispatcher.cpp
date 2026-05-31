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

void ActionDispatcher::dispatch(const ClientMessage& msg,
                                GameWorld& world,
                                Monitor& monitor) {
    uint8_t opcode = msg.message->opCode();

    // Fantasmas solo pueden moverse y resucitar
    if (!world.canPlayerAct(msg.clientId)) {
        if (opcode != static_cast<uint8_t>(ClientOpCode::MSG_MOVE) &&
            opcode != static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT))
            return;
    }

    auto it = handlers.find(opcode);
    if (it != handlers.end())
        (this->*it->second)(msg.clientId, *msg.message, world, monitor);
    else
        std::cerr << "[Dispatcher] OpCode no manejado: 0x"
                  << std::hex << (int)opcode << std::dec << std::endl;
}

void ActionDispatcher::sendStats(uint32_t id, Player& p, Monitor& monitor) {
    monitor.sendTo(id, std::make_shared<const PlayerStatsMessage>(
        p.getLevel(),
        p.getHp(),    p.getMaxHp(),
        p.getMana(),  p.getMaxMana(),
        p.getExp(),   formulas.calcExpLimit(p.getLevel()),
        p.getGold()));
}

void ActionDispatcher::sendInventory(uint32_t id, Player& p, Monitor& monitor) {
    monitor.sendTo(id, std::make_shared<const InventoryUpdateMessage>(
        p.getInventory().getItems(),
        p.getInventory().getEquippedArray()));
}

void ActionDispatcher::sendDeath(uint32_t id, Player& dead, Monitor& monitor) {
    monitor.sendTo(id, std::make_shared<const PlayerDiedMessage>(id));
    sendInventory(id, dead, monitor);
    sendStats(id, dead, monitor);
}

void ActionDispatcher::handleMove(uint32_t id, const Message& msg,
                                   GameWorld& world, Monitor& monitor) {
    const auto& moveMsg = static_cast<const MoveMessage&>(msg);

    if (world.movePlayer(id, moveMsg.getDirection())) {
        monitor.sendTo(id, std::make_shared<const EntityMoveMessage>(
            static_cast<uint8_t>(id),
            world.getPixelX(id),
            world.getPixelY(id)));
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

    world.giveExperience(id, result.expGained);

    sendStats(id,       attacker, monitor);
    sendStats(targetId, target,   monitor);

    if (attacker.checkAndClearLevelUp())
        monitor.sendTo(id, std::make_shared<const LevelUpMessage>(attacker.getLevel()));

    if (result.killed) {
        world.handlePlayerDeath(targetId, id);
        sendDeath(targetId, target, monitor);
        sendStats(id, attacker, monitor); // por si subio con exp kill
        if (attacker.checkAndClearLevelUp())
            monitor.sendTo(id, std::make_shared<const LevelUpMessage>(attacker.getLevel()));
    }
}

void ActionDispatcher::handleDropItem(uint32_t id, const Message& msg,
                                       GameWorld& world, Monitor& monitor) {
    const auto& dropMsg = static_cast<const DropItemMessage&>(msg);
    Player& p = world.getPlayer(id);

    auto removed = p.getInventory().removeItem(dropMsg.getItemId());
    if (removed) {
        world.addItemOnGround(std::move(*removed), p.getTileX(), p.getTileY());
        sendInventory(id, p, monitor);
    }
}

void ActionDispatcher::handlePickItem(uint32_t id, const Message& msg,
                                       GameWorld& world, Monitor& monitor) {
    Player& p = world.getPlayer(id);

    auto item = world.pickItemAt(p.getTileX(), p.getTileY());
    if (item && p.getInventory().addItem(std::move(*item)))
        sendInventory(id, p, monitor);

    auto gold = world.pickGoldAt(p.getTileX(), p.getTileY());
    if (gold) {
        p.addGold(*gold);
        sendStats(id, p, monitor);
    }
}

void ActionDispatcher::handleMeditate(uint32_t id, const Message& msg,
                                       GameWorld& world, Monitor& monitor) {
    Player& p = world.getPlayer(id);
    if (p.isMeditating())
        p.stopMeditating();
    else
        p.startMeditating();
    sendStats(id, p, monitor);
}

void ActionDispatcher::handleResurrect(uint32_t id, const Message& msg,
                                        GameWorld& world, Monitor& monitor) {
    Player& p = world.getPlayer(id);
    if (!p.isGhost()) return;

    world.resurrectPlayer(id, 6, 7);

    monitor.sendTo(id, std::make_shared<const EntityMoveMessage>(
        static_cast<uint8_t>(id),
        world.getPixelX(id),
        world.getPixelY(id)));
    sendStats(id, p, monitor);
}

void ActionDispatcher::handleEquipItem(uint32_t id, const Message& msg,
                                        GameWorld& world, Monitor& monitor) {
    const auto& equipMsg = static_cast<const EquipItemMessage&>(msg);
    Player& p = world.getPlayer(id);

    const Item* item = p.getInventory().findItem(equipMsg.getItemId());
    if (!item) return;

    if (item->slot == ItemSlot::CONSUMABLE) {
        Item copy = *item;
        p.getInventory().removeItem(equipMsg.getItemId());
        effects.apply(copy, p, nullptr);
        sendStats(id, p, monitor);
        sendInventory(id, p, monitor);
        return;
    }

    if (p.getInventory().equipItem(equipMsg.getItemId()))
        sendInventory(id, p, monitor);
}