#include "combatHandler.h"

namespace
{
    void sendCombatChat(uint32_t clientId,
                        const std::string &text,
                        ChatMsgType type,
                        Monitor &monitor)
    {
        monitor.sendTo(clientId,
                       std::make_shared<const ChatNotificationMessage>(text, type));
    }
}

CombatHandler::CombatHandler(const toml::table &config,
                             ClanManager &clanManager)
    : formulas(config), combatSystem(config), itemEffectHandler(),
      resolver(combatSystem, itemEffectHandler, formulas), clanManager(clanManager)
{
}

void CombatHandler::sendStats(uint32_t clientId,
                              Player &player,
                              Monitor &monitor)
{
    monitor.sendTo(clientId,
                   std::make_shared<const PlayerStatsMessage>(
                       player.getLevel(),
                       player.getHp(), player.getMaxHp(),
                       player.getMana(), player.getMaxMana(),
                       player.getExp(),
                       formulas.calcExpLimit(player.getLevel()),
                       player.getGold()));
}

void CombatHandler::sendInventory(uint32_t clientId,
                                  Player &player,
                                  Monitor &monitor)
{
    monitor.sendTo(clientId,
                   std::make_shared<const InventoryUpdateMessage>(
                       player.getInventory().getItems(),
                       player.getInventory().getInventorySlots(),
                       player.getInventory().getEquippedArray()));
}

void CombatHandler::sendLevelUpIfNeeded(uint32_t clientId,
                                        Player &player,
                                        Monitor &monitor)
{
    if (!player.checkAndClearLevelUp())
        return;
    monitor.broadcast(std::make_shared<const LevelUpMessage>(
        clientId, static_cast<uint8_t>(player.getLevel())));
    monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
        clientId,
        static_cast<uint16_t>(player.getHp()),
        static_cast<uint16_t>(player.getMaxHp())));
}

PlayerAttackVisualType CombatHandler::resolveAttackVisualType(
    const Player &attacker) const
{
    const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
    if (weapon == nullptr)
        return PlayerAttackVisualType::Physical;
    if (weapon->slot == ItemSlot::STAFF)
        return PlayerAttackVisualType::Magic;
    if (weapon->stats.isRanged)
        return PlayerAttackVisualType::Ranged;
    return PlayerAttackVisualType::Physical;
}

void CombatHandler::broadcastPlayerAttackVisual(uint32_t attackerId,
                                                uint32_t targetId,
                                                const Player &attacker,
                                                Monitor &monitor)
{
    monitor.broadcast(std::make_shared<const PlayerAttackVisualMessage>(
        attackerId, targetId, resolveAttackVisualType(attacker)));
}

void CombatHandler::handle(uint32_t clientId,
                           const Message &msg,
                           GameWorld &world,
                           Monitor &monitor)
{
    const AttackMessage &attackMsg = static_cast<const AttackMessage &>(msg);
    const uint32_t targetId = attackMsg.getTargetId();

    if (!world.hasPlayer(clientId))
        return;

    Player &attacker = world.getPlayer(clientId);
    if (!attacker.isAlive() || attacker.isGhost())
        return;

    if (world.hasPlayer(targetId))
    {
        handleAttackPlayer(clientId, targetId, world, monitor);
        return;
    }

    if (world.hasNpc(targetId))
    {
        handleAttackNpc(clientId, targetId, world, monitor);
        return;
    }
}

void CombatHandler::handleAttackPlayer(uint32_t attackerId,
                                       uint32_t targetId,
                                       GameWorld &world,
                                       Monitor &monitor)
{
    if (attackerId == targetId)
        return;

    Player &attacker = world.getPlayer(attackerId);
    Player &target = world.getPlayer(targetId);

    if (!attacker.isAlive() || attacker.isGhost())
        return;
    if (!target.isAlive() || target.isGhost())
        return;

    const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
    if (weapon != nullptr && weapon->effect == ItemEffect::HEAL)
    {
        if (itemEffectHandler.apply(*weapon, attacker, &target))
        {
            sendStats(attackerId, attacker, monitor);
            sendStats(targetId, target, monitor);
        }
        return;
    }

    CombatSystem::Result result = combatSystem.attackPlayer(attacker, target, world);

    if (!result.valid)
    {
        if (result.failReason == CombatSystem::Result::FailReason::FRIENDLY_FIRE)
            sendCombatChat(attackerId, "No podés atacar a tus aliados.",
                           ChatMsgType::INFO, monitor);
        else if (result.failReason == CombatSystem::Result::FailReason::NO_MANA)
            sendCombatChat(attackerId,
                           "No tenés mana suficiente para realizar ese hechizo.",
                           ChatMsgType::INFO, monitor);
        else if (result.failReason == CombatSystem::Result::FailReason::LEVEL_TOO_LOW)
            sendCombatChat(attackerId,
                           "No podés atacar a jugadores de nivel bajo.",
                           ChatMsgType::INFO, monitor);
        else if (result.failReason == CombatSystem::Result::FailReason::LEVEL_DIFF_TOO_HIGH)
            sendCombatChat(attackerId,
                           "La diferencia de nivel es demasiado grande para atacar.",
                           ChatMsgType::INFO, monitor);
        sendStats(attackerId, attacker, monitor);
        return;
    }

    std::optional<std::pair<std::string, bool>> targetClanInfo =
        clanManager.findClanInfoForMember(target.getName());
    if (targetClanInfo.has_value())
    {
        const std::string alertMsg =
            "¡Nuestro aliado " + target.getName() + " está siendo atacado!";
        std::vector<uint32_t> localAllies =
            world.getOnlineClanMemberIds(targetClanInfo->first);
        for (uint32_t allyId : localAllies)
        {
            if (allyId == targetId)
                continue;
            sendCombatChat(allyId, alertMsg, ChatMsgType::CLAN, monitor);
        }
    }

    if (!result.dodged)
    {
        monitor.sendTo(attackerId,
                       std::make_shared<const CombatLogMessage>(std::to_string(targetId)));
        broadcastPlayerAttackVisual(attackerId, targetId, attacker, monitor);
    }

    if (result.dodged)
    {
        sendCombatChat(attackerId, "¡Tu ataque fue esquivado!",
                       ChatMsgType::INFO, monitor);
        sendCombatChat(targetId, "¡Esquivaste el ataque!",
                       ChatMsgType::INFO, monitor);
        sendStats(attackerId, attacker, monitor);
        sendStats(targetId, target, monitor);
        return;
    }

    if (result.killed || target.getHp() <= 0)
    {
        GameWorld::DeathResult deathResult =
            world.handlePlayerDeath(targetId, attackerId);

        if (deathResult.excessGold > 0)
            monitor.broadcast(std::make_shared<const GoldOnGroundMessage>(
                deathResult.goldInstanceId,
                deathResult.excessGold,
                deathResult.tileX,
                deathResult.tileY));

        for (const Item &item : deathResult.droppedItems)
            monitor.broadcast(std::make_shared<const ItemOnGroundMessage>(
                item, deathResult.tileX, deathResult.tileY));

        sendCombatChat(attackerId,
                       "¡Mataste a " + target.getName() + "!",
                       ChatMsgType::DAMAGE_DEALT, monitor);
        sendCombatChat(targetId,
                       "¡Fuiste asesinado por " + attacker.getName() + "!",
                       ChatMsgType::DAMAGE_TAKEN, monitor);

        sendStats(attackerId, attacker, monitor);
        sendLevelUpIfNeeded(attackerId, attacker, monitor);
        sendInventory(targetId, target, monitor);

        monitor.sendTo(targetId, std::make_shared<const PlayerDiedMessage>(targetId));
        monitor.broadcast(std::make_shared<const PlayerDiedMessage>(targetId));
        sendStats(targetId, target, monitor);
        monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
            targetId,
            static_cast<uint16_t>(target.getHp()),
            static_cast<uint16_t>(target.getMaxHp())));
        return;
    }

    sendCombatChat(attackerId,
                   "Causaste " + std::to_string(result.damage) +
                       " pts de daño a " + target.getName() + ".",
                   ChatMsgType::DAMAGE_DEALT, monitor);
    sendCombatChat(targetId,
                   attacker.getName() + " te causó " +
                       std::to_string(result.damage) + " pts de daño.",
                   ChatMsgType::DAMAGE_TAKEN, monitor);

    world.giveExperience(attackerId, result.expGained);

    sendStats(attackerId, attacker, monitor);
    sendStats(targetId, target, monitor);
    sendLevelUpIfNeeded(attackerId, attacker, monitor);
    monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
        targetId,
        static_cast<uint16_t>(target.getHp()),
        static_cast<uint16_t>(target.getMaxHp())));
}

void CombatHandler::handleAttackNpc(uint32_t attackerId,
                                    uint32_t npcId,
                                    GameWorld &world,
                                    Monitor &monitor)
{
    Player &attacker = world.getPlayer(attackerId);

    if (!attacker.isAlive() || attacker.isGhost())
        return;

    const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
    if (weapon != nullptr && weapon->effect == ItemEffect::HEAL)
        return;

    Npc &npc = world.getNpc(npcId);
    if (!npc.isAlive())
        return;
    if (!npc.isHostile())
        return;

    CombatSystem::Result result = combatSystem.attackNpc(attacker, npc, world);

    if (!result.valid)
    {
        if (result.failReason == CombatSystem::Result::FailReason::NO_MANA)
            sendCombatChat(attackerId,
                           "No tenés mana suficiente para realizar ese hechizo.",
                           ChatMsgType::INFO, monitor);
        sendStats(attackerId, attacker, monitor);
        return;
    }

    if (!result.dodged)
    {
        monitor.sendTo(attackerId,
                       std::make_shared<const CombatLogMessage>(std::to_string(npcId)));
        broadcastPlayerAttackVisual(attackerId, npcId, attacker, monitor);
    }

    monitor.broadcast(std::make_shared<const NpcHealthMessage>(
        npcId,
        static_cast<uint16_t>(npc.getHp()),
        static_cast<uint16_t>(npc.getMaxHp())));

    if (result.killed)
    {
        NpcDropResult dropResult = world.handleNpcDeath(npcId, attackerId);

        if (dropResult.hasGold)
            monitor.broadcast(std::make_shared<const GoldOnGroundMessage>(
                dropResult.goldInstanceId,
                dropResult.goldAmount,
                dropResult.tileX,
                dropResult.tileY));

        if (dropResult.hasItem)
            monitor.broadcast(std::make_shared<const ItemOnGroundMessage>(
                dropResult.droppedItem,
                dropResult.tileX,
                dropResult.tileY));

        sendCombatChat(attackerId,
                       "¡Mataste al " + npc.getName() + "!",
                       ChatMsgType::DAMAGE_DEALT, monitor);
        sendStats(attackerId, attacker, monitor);
        sendLevelUpIfNeeded(attackerId, attacker, monitor);
        return;
    }

    if (result.dodged)
    {
        sendCombatChat(attackerId,
                       "¡El " + npc.getName() + " esquivó tu ataque!",
                       ChatMsgType::INFO, monitor);
        sendStats(attackerId, attacker, monitor);
        return;
    }

    sendCombatChat(attackerId,
                   "Causaste " + std::to_string(result.damage) +
                       " pts de daño al " + npc.getName() + ".",
                   ChatMsgType::DAMAGE_DEALT, monitor);

    world.giveExperience(attackerId, result.expGained);
    npc.setTargetId(attackerId);
    npc.setState(NpcState::CHASING);

    sendStats(attackerId, attacker, monitor);
    sendLevelUpIfNeeded(attackerId, attacker, monitor);
}