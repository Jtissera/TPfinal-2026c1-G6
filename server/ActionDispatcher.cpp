#include "ActionDispatcher.h"

#include "common/network/messages/client/combat/enemyHitPlayerMessage.h"


ActionDispatcher::ActionDispatcher() {
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_MOVE)]       = &ActionDispatcher::handleMove;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_ATTACK)]     = &ActionDispatcher::handleAttack;
    handlers[static_cast<uint8_t>(ClientOpCode::MSG_ENEMY_HIT_PLAYER)] = &ActionDispatcher::handleEnemyHitPlayer;
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

void ActionDispatcher::handleMove(uint32_t id, const Message& msg,GameWorld& world, Monitor& monitor) {

    const auto& moveMsg = static_cast<const MoveMessage&>(msg);

    // El server valida y aplica el movimiento en el GameWorld.
    if (world.movePlayer(id, moveMsg.getDirection())) {
        //mensaje de movimiento
        auto movementMsg = std::make_shared<const EntityMoveMessage>(
            static_cast<uint8_t>(id),
            world.getPixelX(id),
            world.getPixelY(id),
            moveMsg.getDirection(),
            true
        );

        //odos los clientes de la sala deben recibir la nueva posición de este jugador.
        monitor.broadcast(movementMsg);

        std::cout << "[SERVER MOVE] broadcast playerId="
                  << id
                  << " pos=("
                  << world.getPixelX(id)
                  << ", "
                  << world.getPixelY(id)
                  << ")"
                  << std::endl;
    }
}
void ActionDispatcher::handleAttack(
    uint32_t id,
    const Message& msg,
    GameWorld& world,
    Monitor& monitor
) {
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