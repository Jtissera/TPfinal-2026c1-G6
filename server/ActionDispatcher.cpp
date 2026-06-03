#include "ActionDispatcher.h"

#include "common/network/messages/client/combat/enemyHitPlayerMessage.h"
#include "common/network/messages/client/inventory/unequipSlotMessage.h"
#include "common/network/messages/server/player/playerEquipmentUpdateMessage.h"


ActionDispatcher::ActionDispatcher() {
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MOVE)]       = &ActionDispatcher::handleMove;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_ATTACK)]     = &ActionDispatcher::handleAttack;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_ENEMY_HIT_PLAYER)] = &ActionDispatcher::handleEnemyHitPlayer;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_PICK_ITEM)]  = &ActionDispatcher::handlePickItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_DROP_ITEM)]  = &ActionDispatcher::handleDropItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_EQUIP_ITEM)] = &ActionDispatcher::handleEquipItem;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MEDITATE)]   = &ActionDispatcher::handleMeditate;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_RESURRECT)]  = &ActionDispatcher::handleResurrect;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_UNEQUIP_SLOT)] =&ActionDispatcher::handleUnequipSlot;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_USE_ITEM)] = &ActionDispatcher::handleUseItem;
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

void ActionDispatcher::handleAttack(uint32_t id,const Message& msg,GameWorld& world,Monitor& monitor) {
    const auto& attackMsg = static_cast<const AttackMessage&>(msg);
    const uint32_t targetId = attackMsg.getTargetId();

    if (!world.hasPlayer(id)) {
        return;
    }

    Player& attacker = world.getPlayer(id);
    const Item* weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);

    // ============================================================
    // PvP: player contra player
    // ============================================================
    if (world.hasPlayer(targetId)) {
        Player& target = world.getPlayer(targetId);

        if (weapon != nullptr && weapon->effect == ItemEffect::HEAL) {
            if (effects.apply(*weapon, attacker, &target)) {
                sendStats(id, attacker, monitor);
                sendStats(targetId, target, monitor);
            }

            return;
        }

        auto result = combat.attackPlayer(attacker, target);

        if (!result.valid) {
            return;
        }

        if (result.dodged) {
            sendStats(id, attacker, monitor);
            sendStats(targetId, target, monitor);
            return;
        }

        world.giveExperience(id, result.expGained);

        sendStats(id, attacker, monitor);
        sendStats(targetId, target, monitor);

        if (attacker.checkAndClearLevelUp()) {
            monitor.sendTo(
                id,
                std::make_shared<const LevelUpMessage>(attacker.getLevel())
            );
        }

        if (result.killed) {
            world.handlePlayerDeath(targetId, id);

            sendDeath(targetId, target, monitor);

            sendStats(id, attacker, monitor);

            if (attacker.checkAndClearLevelUp()) {
                monitor.sendTo(
                    id,
                    std::make_shared<const LevelUpMessage>(attacker.getLevel())
                );
            }
        }

        return;
    }

    // ============================================================
    // PvE: player contra NPC
    // ============================================================
    if (world.hasNpc(targetId)) {
        // Hechizos/ítems de curación no se procesan como ataque PvE.
        if (weapon != nullptr && weapon->effect == ItemEffect::HEAL) {
            return;
        }

        Npc& npc = world.getNpc(targetId);

        // Si el NPC es pasivo, no es atacable.
        if (!npc.isHostile()) {
            return;
        }

        // Esto valida rango con getAttackRange(),
        // calcula daño con getWeaponDamageMin/Max(),
        // aplica esquive/defensa y resta vida.
        auto result = combat.attack(attacker, npc);

        if (!result.valid) {
            return;
        }
        monitor.broadcast(std::make_shared<const NpcHealthMessage>(targetId,npc.getHp(),npc.getMaxHp())
);

        if (result.dodged) {
            sendStats(id, attacker, monitor);
            return;
        }

        world.giveExperience(id, result.expGained);

        // Si sigue vivo, el NPC debe perseguir al atacante.
        if (!result.killed) {
            npc.setTargetId(id);
            npc.setState(NpcState::CHASING);
        }

        sendStats(id, attacker, monitor);

        if (attacker.checkAndClearLevelUp()) {
            monitor.sendTo(id,std::make_shared<const LevelUpMessage>(attacker.getLevel()));
        }

        if (result.killed) {
            sendStats(id, attacker, monitor);
            if (attacker.checkAndClearLevelUp()) {
                monitor.sendTo(id,std::make_shared<const LevelUpMessage>(attacker.getLevel()));
            }
        }

        return;
    }

    return;
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
    monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(id,buildEquipmentDto(player)));
}

void ActionDispatcher::handleEnemyHitPlayer(
    uint32_t id,
    const Message& msg,
    GameWorld& world,
    Monitor& monitor
) {
    const auto& hitMsg = static_cast<const EnemyHitPlayerMessage&>(msg);

    if (!world.hasPlayer(id)) {
        return;
    }

    Player& player = world.getPlayer(id);

    if (player.isGhost()) {
        return;
    }

    // Daño temporal para enemigo local del cliente.
    // Luego conviene validarlo contra NPC real del server.
    const int16_t damage = 5;

    player.takeDamage(damage);

    std::cout << "[SERVER][ENEMY HIT] enemyId="
              << hitMsg.getEnemyId()
              << " playerId=" << id
              << " damage=" << damage
              << " hp=" << player.getHp()
              << "/" << player.getMaxHp()
              << std::endl;

    sendStats(id, player, monitor);

    if (!player.isAlive()) {
        world.handlePlayerDeath(id, 0);
        sendDeath(id, player, monitor);
    }
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
    monitor.broadcast(std::make_shared<const PlayerEquipmentUpdateMessage>(id,buildEquipmentDto(player)));
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

EquipmentDto ActionDispatcher::buildEquipmentDto(const Player& player) const {
    // DTO visual que se mandará a otros clientes.
    // Cada campo es catalogId, no instanceId.
    EquipmentDto dto{};

    // Obtenemos el inventario real del jugador en server.
    const Inventory& inventory = player.getInventory();

    // Si hay arma equipada, mandamos su catalogId.
    if (const Item* weapon = inventory.getEquipped(EquipSlot::HAND)) {
        dto.weaponCatalogId = weapon->catalogId;
    }

    // Si hay armadura equipada, mandamos su catalogId.
    if (const Item* armor = inventory.getEquipped(EquipSlot::ARMOR)) {
        dto.armorCatalogId = armor->catalogId;
    }

    // Si hay casco/capucha equipada, mandamos su catalogId.
    if (const Item* helmet = inventory.getEquipped(EquipSlot::HELMET)) {
        dto.helmetCatalogId = helmet->catalogId;
    }

    // Si hay escudo equipado, mandamos su catalogId.
    if (const Item* shield = inventory.getEquipped(EquipSlot::SHIELD)) {
        dto.shieldCatalogId = shield->catalogId;
    }

    return dto;
}