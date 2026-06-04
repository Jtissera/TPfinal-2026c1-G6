#include "ActionDispatcher.h"

#include "common/network/messages/client/inventory/unequipSlotMessage.h"
#include "common/network/messages/server/player/playerEquipmentUpdateMessage.h"
#include "server/game/equipmentDtoFactory.h"

ActionDispatcher::ActionDispatcher() {
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MOVE)]       = &ActionDispatcher::handleMove;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_PICK_ITEM)]  = &ActionDispatcher::handlePickItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_DROP_ITEM)]  = &ActionDispatcher::handleDropItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_EQUIP_ITEM)] = &ActionDispatcher::handleEquipItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_UNEQUIP_SLOT)] =&ActionDispatcher::handleUnequipSlot;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_USE_ITEM)] = &ActionDispatcher::handleUseItem;

    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MEDITATE)]   = &ActionDispatcher::handleMeditate;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT)]  = &ActionDispatcher::handleResurrect;

    handlers[static_cast<uint8_t>(ClientOpCode::MSG_ATTACK)] =&ActionDispatcher::handleAttack;
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
        p.getInventory().getInventorySlots(),
        p.getInventory().getEquippedArray()));
}

void ActionDispatcher::sendDeath(uint32_t id, Player& dead, Monitor& monitor) {
    monitor.sendTo(id, std::make_shared<const PlayerDiedMessage>(id));
    sendInventory(id, dead, monitor);
    sendStats(id, dead, monitor);
}

void ActionDispatcher::handleMove(uint32_t id, const Message& msg,GameWorld& world, Monitor& monitor) {

    const auto& moveMsg = static_cast<const MoveMessage&>(msg);
    const Direction direction = moveMsg.getDirection();
    const bool moving = moveMsg.isMoving();

    if (!moving) {
        monitor.broadcast(std::make_shared<const EntityMoveMessage>(
            static_cast<uint8_t>(id),
            world.getPixelX(id),
            world.getPixelY(id),
            direction,
            false
        ));

        return;
    }

    if (world.movePlayer(id, direction)) {
        monitor.broadcast(std::make_shared<const EntityMoveMessage>(
            static_cast<uint8_t>(id),
            world.getPixelX(id),
            world.getPixelY(id),
            direction,
            true
        ));
    }
}



void ActionDispatcher::handleDropItem(uint32_t id, const Message& msg,GameWorld& world, Monitor& monitor) {
    const auto& dropMsg = static_cast<const DropItemMessage&>(msg);
    Player& p = world.getPlayer(id);

    auto removed = p.getInventory().removeItem(dropMsg.getItemId());
    if (removed) {
        world.addItemOnGround(std::move(*removed), p.getTileX(), p.getTileY());
        sendInventory(id, p, monitor);
    }
}

void ActionDispatcher::handlePickItem(uint32_t id, const Message& msg,GameWorld& world, Monitor& monitor) {
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

void ActionDispatcher::handleMeditate(uint32_t id, const Message& msg,GameWorld& world, Monitor& monitor) {
    Player& p = world.getPlayer(id);
    if (p.isMeditating())
        p.stopMeditating();
    else
        p.startMeditating();
    sendStats(id, p, monitor);
}

void ActionDispatcher::handleResurrect(uint32_t id, const Message& msg,GameWorld& world, Monitor& monitor) {
    std::cout << "[SERVER RESURRECT] client=" << id << std::endl;

    Player& p = world.getPlayer(id);

    std::cout << "[SERVER RESURRECT] isGhost=" << p.isGhost()
              << " hp=" << p.getHp()
              << std::endl;

    world.resurrectPlayer(id, 6, 7);

    std::cout << "[SERVER RESURRECT] after resurrect hp="
              << p.getHp()
              << std::endl;

    monitor.sendTo(id, std::make_shared<const EntityMoveMessage>(
        static_cast<uint8_t>(id),
        world.getPixelX(id),
        world.getPixelY(id),
        Direction::DOWN,
        false
    ));

    sendStats(id, p, monitor);
}

void ActionDispatcher::handleEquipItem(uint32_t id,const Message& msg,GameWorld& world,Monitor& monitor) {
    const auto& equipMsg = static_cast<const EquipItemMessage&>(msg);
    const uint32_t itemInstanceId = equipMsg.getItemInstanceId();

    std::cout << "[SERVER EQUIP] client="
              << id
              << " itemInstanceId="
              << itemInstanceId
              << std::endl;

    if (!world.hasPlayer(id)) {
        std::cerr << "[SERVER EQUIP] jugador inexistente id="
                  << id
                  << std::endl;
        return;
    }

    Player& player = world.getPlayer(id);

    const bool equipped = player.getInventory().equipItem(itemInstanceId);

    if (!equipped) {
        std::cerr << "[SERVER EQUIP] no se pudo equipar itemInstanceId="
                  << itemInstanceId
                  << std::endl;

        sendInventory(id, player, monitor);
        return;
    }

    std::cout << "[SERVER EQUIP] equipado correctamente itemInstanceId="
              << itemInstanceId
              << std::endl;

    sendInventory(id, player, monitor);
    std::cout << "[SERVER EQUIP UPDATE] broadcast playerId="<< id << std::endl;
    monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(id,buildEquipmentDtoFromPlayer(player)));
}


void ActionDispatcher::handleUnequipSlot(uint32_t id,const Message& msg,GameWorld& world,Monitor& monitor) {
    const auto& unequipMsg = static_cast<const UnequipSlotMessage&>(msg);
    const EquipSlot slot = unequipMsg.getSlot();

    if (!world.hasPlayer(id)) {
        return;
    }

    Player& player = world.getPlayer(id);

    const bool ok = player.getInventory().unequipSlot(slot);

    std::cout << "[SERVER UNEQUIP] client="
              << id
              << " slot="
              << static_cast<int>(slot)
              << " ok="
              << ok
              << std::endl;

    sendInventory(id, player, monitor);
    std::cout << "[SERVER EQUIP UPDATE] broadcast playerId="
          << id
          << std::endl;
    monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(id,buildEquipmentDtoFromPlayer(player)));
}

void ActionDispatcher::handleUseItem(
    uint32_t id,
    const Message& msg,
    GameWorld& world,
    Monitor& monitor
) {
    const auto& useMsg = static_cast<const UseItemMessage&>(msg);
    const uint32_t itemInstanceId = useMsg.getItemInstanceId();

    if (!world.hasPlayer(id)) {
        std::cerr << "[SERVER USE ITEM] jugador inexistente id="
                  << id
                  << std::endl;
        return;
    }

    Player& player = world.getPlayer(id);

    const Item* item = player.getInventory().findItem(itemInstanceId);

    if (item == nullptr) {
        std::cerr << "[SERVER USE ITEM] item inexistente instanceId="
                  << itemInstanceId
                  << std::endl;

        sendInventory(id, player, monitor);
        return;
    }

    bool used = false;

    if (item->slot == ItemSlot::CONSUMABLE) {
        if (item->stats.healAmount > 0) {
            player.heal(item->stats.healAmount);
            used = true;
        }

        if (item->stats.manaAmount > 0) {
            player.restoreMana(item->stats.manaAmount);
            used = true;
        }
    }

    if (!used) {
        std::cerr << "[SERVER USE ITEM] item no consumible o sin efecto. instanceId="
                  << itemInstanceId
                  << std::endl;

        sendInventory(id, player, monitor);
        sendStats(id, player, monitor);
        return;
    }

    player.getInventory().removeItem(itemInstanceId);

    std::cout << "[SERVER USE ITEM] client="
              << id
              << " itemInstanceId="
              << itemInstanceId
              << " usado correctamente"
              << std::endl;

    sendStats(id, player, monitor);
    sendInventory(id, player, monitor);
}

void ActionDispatcher::handleAttack(uint32_t id,const Message& msg,GameWorld& world,Monitor& monitor) {

    const auto& attackMsg = static_cast<const AttackMessage&>(msg);
    const uint32_t targetId = attackMsg.getTargetId();

    std::cout << "[SERVER ATTACK] attackerId="
          << id
          << " targetId="
          << targetId
          << std::endl;

    if (!world.hasPlayer(id)) {
        std::cout << "[SERVER ATTACK] attacker inexistente id="
          << id
          << std::endl;
        return;
    }

    Player& attacker = world.getPlayer(id);

    if (!attacker.isAlive() || attacker.isGhost()) {
        std::cout << "[SERVER ATTACK] attacker muerto/fantasma id="
          << id
          << std::endl;
        return;
    }

    if (world.hasPlayer(targetId)) {
        std::cout << "[SERVER ATTACK] target es PLAYER id="
          << targetId
          << std::endl;
        handleAttackPlayer(id, targetId, world, monitor);
        return;
    }

    if (world.hasNpc(targetId)) {
        std::cout << "[SERVER ATTACK] target es NPC id="
          << targetId
          << std::endl;
        handleAttackNpc(id, targetId, world, monitor);
        return;
    }

    std::cout << "[SERVER ATTACK] target inexistente targetId="<< targetId<< " attackerId="<< id << std::endl;
}

void ActionDispatcher::handleAttackPlayer(uint32_t attackerId,uint32_t targetId,GameWorld& world,Monitor& monitor) {
    std::cout << "[SERVER PVP] attackerId="
          << attackerId
          << " targetId="
          << targetId
          << std::endl;
    if (attackerId == targetId) {
        return;
    }

    Player& attacker = world.getPlayer(attackerId);
    Player& target = world.getPlayer(targetId);

    if (!attacker.isAlive() || attacker.isGhost()) {
        return;
    }

    if (!target.isAlive() || target.isGhost()) {
        return;
    }

    const Item* weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);

    if (weapon != nullptr && weapon->effect == ItemEffect::HEAL) {
        if (effects.apply(*weapon, attacker, &target)) {
            sendStats(attackerId, attacker, monitor);
            sendStats(targetId, target, monitor);
        }

        return;
    }

    auto result = combat.attackPlayer(attacker, target);

    std::cout << "[SERVER PVP RESULT] valid="
          << result.valid
          << " dodged="
          << result.dodged
          << " killed="
          << result.killed
          << " attackerMana="
          << attacker.getMana()
          << "/"
          << attacker.getMaxMana()
          << " targetHp="
          << target.getHp()
          << "/"
          << target.getMaxHp()
          << std::endl;

    if (!result.valid) {
        sendStats(attackerId, attacker, monitor);
        return;
    }

    sendStats(attackerId, attacker, monitor);
    sendStats(targetId, target, monitor);

    if (result.dodged) {
        return;
    }

    world.giveExperience(attackerId, result.expGained);

    sendStats(attackerId, attacker, monitor);
    sendLevelUpIfNeeded(attackerId, attacker, monitor);

    if (result.killed) {
        world.handlePlayerDeath(targetId, attackerId);

        sendDeath(targetId, target, monitor);

        sendStats(attackerId, attacker, monitor);
        sendLevelUpIfNeeded(attackerId, attacker, monitor);
    }
}

void ActionDispatcher::handleAttackNpc(uint32_t attackerId,uint32_t npcId,GameWorld& world,Monitor& monitor) {
    Player& attacker = world.getPlayer(attackerId);

    if (!attacker.isAlive() || attacker.isGhost()) {
        return;
    }

    const Item* weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);

    // Hechizos/ítems de curación no se procesan como ataque PvE.
    if (weapon != nullptr && weapon->effect == ItemEffect::HEAL) {
        return;
    }

    Npc& npc = world.getNpc(npcId);

    if (!npc.isAlive()) {
        return;
    }

    if (!npc.isHostile()) {
        return;
    }

    auto result = combat.attackNpc(attacker, npc);

    if (!result.valid) {
        sendStats(attackerId, attacker, monitor);
        return;
    }

    monitor.broadcast(std::make_shared<const NpcHealthMessage>(npcId,npc.getHp(),npc.getMaxHp()));

    sendStats(attackerId, attacker, monitor);

    if (result.dodged) {
        return;
    }

    world.giveExperience(attackerId, result.expGained);

    if (!result.killed) {
        npc.setTargetId(attackerId);
        npc.setState(NpcState::CHASING);
    }

    sendStats(attackerId, attacker, monitor);
    sendLevelUpIfNeeded(attackerId, attacker, monitor);

    if (result.killed) {
        sendStats(attackerId, attacker, monitor);
        sendLevelUpIfNeeded(attackerId, attacker, monitor);
    }
}

void ActionDispatcher::sendLevelUpIfNeeded(uint32_t playerId,Player& player,Monitor& monitor) {
    if (!player.checkAndClearLevelUp()) {
        return;
    }

    monitor.sendTo(
        playerId,
        std::make_shared<const LevelUpMessage>(player.getLevel())
    );
}