#include "combatHandler.h"

namespace {
void sendCombatChat(uint32_t clientId, const std::string &text,
                    ChatMsgType type, Monitor &monitor) {
  monitor.sendTo(clientId,
                 std::make_shared<const ChatNotificationMessage>(text, type));
}
} // namespace

CombatHandler::CombatHandler(const toml::table &config,
                             ClanManager &clanManager)
    : formulas(config), combatSystem(config), itemEffectHandler(),
      resolver(combatSystem, itemEffectHandler, formulas),
      clanManager(clanManager) {}

void CombatHandler::sendStats(uint32_t clientId, Player &player,
                              Monitor &monitor) {
  monitor.sendTo(clientId,
                 std::make_shared<const PlayerStatsMessage>(
                     player.getLevel(), player.getHp(), player.getMaxHp(),
                     player.getMana(), player.getMaxMana(), player.getExp(),
                     formulas.calcExpLimit(player.getLevel()),
                     player.getGold()));
}

void CombatHandler::sendInventory(uint32_t clientId, Player &player,
                                  Monitor &monitor) {
  monitor.sendTo(clientId, std::make_shared<const InventoryUpdateMessage>(
                               player.getInventory().getItems(),
                               player.getInventory().getInventorySlots(),
                               player.getInventory().getEquippedArray()));
}

void CombatHandler::sendLevelUpIfNeeded(uint32_t clientId, Player &player,
                                        Monitor &monitor) {
  if (!player.checkAndClearLevelUp())
    return;
  monitor.broadcast(std::make_shared<const LevelUpMessage>(
      clientId, static_cast<uint8_t>(player.getLevel())));
  monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
      clientId, static_cast<uint16_t>(player.getHp()),
      static_cast<uint16_t>(player.getMaxHp())));
}

PlayerAttackVisualType
CombatHandler::resolveAttackVisualType(const Player &attacker) const {
  const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
  if (weapon == nullptr)
    return PlayerAttackVisualType::Physical;
  if (weapon->effect == ItemEffect::HEAL)
    return PlayerAttackVisualType::Heal;
  if (weapon->slot == ItemSlot::STAFF)
    return PlayerAttackVisualType::Magic;
  if (weapon->stats.isRanged)
    return PlayerAttackVisualType::Ranged;
  return PlayerAttackVisualType::Physical;
}

void CombatHandler::broadcastPlayerAttackVisual(uint32_t attackerId,
                                                uint32_t targetId,
                                                const Player &attacker,
                                                Monitor &monitor) {
  const PlayerAttackVisualType visualType = resolveAttackVisualType(attacker);
  const std::string effectId = resolveAttackEffectId(attacker);

  monitor.broadcast(std::make_shared<const PlayerAttackVisualMessage>(
      attackerId, targetId, visualType, effectId));
}

void CombatHandler::handle(uint32_t clientId, const Message &msg,
                           GameWorld &world, Monitor &monitor) {
  const AttackMessage &attackMsg = static_cast<const AttackMessage &>(msg);
  const uint32_t targetId = attackMsg.getTargetId();

  if (!world.hasPlayer(clientId))
    return;

  Player &attacker = world.getPlayer(clientId);
  if (!attacker.isAlive() || attacker.isGhost())
    return;

  if (world.hasPlayer(targetId)) {
    handleAttackPlayer(clientId, targetId, world, monitor);
    return;
  }

  if (world.hasNpc(targetId)) {
    handleAttackNpc(clientId, targetId, world, monitor);
    return;
  }
}
void CombatHandler::handleAttackPlayer(uint32_t attackerId, uint32_t targetId,
                                       GameWorld &world, Monitor &monitor) {
  // Obtenemos al atacante desde el mundo.
  // Si llegó hasta acá, el targetId ya corresponde a un jugador existente.
  Player &attacker = world.getPlayer(attackerId);

  // Un atacante muerto o fantasma no puede atacar ni curar.
  if (!attacker.isAlive() || attacker.isGhost()) {
    return;
  }

  const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
  const bool isHealWeapon = weapon != nullptr && weapon->effect == ItemEffect::HEAL;


  if (attackerId == targetId && !isHealWeapon) {
    return;
  }

  Player &target = world.getPlayer(targetId);


  if (!target.isAlive() || target.isGhost() || target.getHp() == 0) {
    return;
  }

  if (isHealWeapon) {

    if (itemEffectHandler.apply(*weapon, attacker, &target)) {

      if (attackerId == targetId) {
        sendStats(attackerId, attacker, monitor);
      } else {
        // El atacante gastó maná.
        sendStats(attackerId, attacker, monitor);

        // El target recuperó vida.
        sendStats(targetId, target, monitor);
      }


      monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
          targetId,
          static_cast<uint16_t>(target.getHp()),
          static_cast<uint16_t>(target.getMaxHp())));

      // Avisamos a todos el efecto visual de curación.
      // El cliente debería mostrar el visual_effect de la flauta.
      monitor.broadcast(std::make_shared<const PlayerAttackVisualMessage>(
          attackerId,
          targetId,
          PlayerAttackVisualType::Heal,
          weapon->stats.visualEffectId));
    } else {
      sendCombatChat(attackerId,
                     "No tenés mana suficiente para curar.",
                     ChatMsgType::INFO,
                     monitor);

      // Reenviamos stats para dejar el HUD sincronizado.
      sendStats(attackerId, attacker, monitor);
    }

    // Como era curación, no seguimos al combate normal.
    return;
  }

  // A partir de acá sí estamos procesando un ataque hostil.
  // Por eso recién ahora validamos zona segura.
  const Tile &attackerTile =
      world.getTileAt(attacker.getTileX(), attacker.getTileY());

  const Tile &targetTile =
      world.getTileAt(target.getTileX(), target.getTileY());

  // En zona segura no se puede atacar a otros jugadores.
  if (attackerTile.zone == ZoneType::SAFE ||
      targetTile.zone == ZoneType::SAFE) {
    sendCombatChat(attackerId,
                   "No podés atacar jugadores en zona segura.",
                   ChatMsgType::INFO,
                   monitor);
    return;
  }

void CombatHandler::handleAttackNpc(uint32_t attackerId, uint32_t npcId,
                                    GameWorld &world, Monitor &monitor) {
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

  if (!result.valid) {
    if (result.failReason == CombatSystem::Result::FailReason::NO_MANA)
      sendCombatChat(attackerId,
                     "No tenés mana suficiente para realizar ese hechizo.",
                     ChatMsgType::INFO, monitor);
    sendStats(attackerId, attacker, monitor);
    return;
  }

  if (!result.dodged) {
    monitor.sendTo(attackerId, std::make_shared<const CombatLogMessage>(
                                   std::to_string(npcId)));
    broadcastPlayerAttackVisual(attackerId, npcId, attacker, monitor);
  }

  monitor.broadcast(std::make_shared<const NpcHealthMessage>(
      npcId, static_cast<uint16_t>(npc.getHp()),
      static_cast<uint16_t>(npc.getMaxHp())));

  if (result.killed) {
    NpcDropResult dropResult = world.handleNpcDeath(npcId, attackerId);

    if (dropResult.hasGold)
      monitor.broadcast(std::make_shared<const GoldOnGroundMessage>(
          dropResult.goldInstanceId, dropResult.goldAmount, dropResult.tileX,
          dropResult.tileY));

    if (dropResult.hasItem)
      monitor.broadcast(std::make_shared<const ItemOnGroundMessage>(
          dropResult.droppedItem, dropResult.tileX, dropResult.tileY));

    sendCombatChat(attackerId, "¡Mataste al " + npc.getName() + "!",
                   ChatMsgType::DAMAGE_DEALT, monitor);
    sendStats(attackerId, attacker, monitor);
    sendLevelUpIfNeeded(attackerId, attacker, monitor);
    return;
  }

  if (result.dodged) {
    sendCombatChat(attackerId, "¡El " + npc.getName() + " esquivó tu ataque!",
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

std::string CombatHandler::resolveAttackEffectId(const Player &attacker) const {
  const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);
  if (weapon == nullptr)
    return "";

  std::cout << "[SERVER EFFECT] item='" << weapon->catalogId << "' typeName='"
            << weapon->typeName << "' effectId='"
            << weapon->stats.visualEffectId << "'" << std::endl;

  return weapon->stats.visualEffectId;
}