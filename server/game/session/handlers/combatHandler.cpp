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

  Player &attacker = world.getPlayer(attackerId);

  // Si el atacante está muerto o fantasma, no puede atacar.
  if (!attacker.isAlive() || attacker.isGhost()) {
    std::cout << "[PVP DEBUG] cortado: attacker muerto/fantasma attackerId="
              << attackerId
              << std::endl;
    return;
  }

  // Obtenemos el arma equipada.
  const Item *weapon = attacker.getInventory().getEquipped(EquipSlot::HAND);

  // Detectamos si el arma es de curación.
  const bool isHealWeapon =
      weapon != nullptr && weapon->effect == ItemEffect::HEAL;

  std::cout << "[PVP DEBUG] weapon="
            << (weapon ? static_cast<int>(weapon->catalogId) : -1)
            << " isHealWeapon="
            << isHealWeapon
            << std::endl;

  // Si se apunta a sí mismo sin arma de curación, bloqueamos.
  if (attackerId == targetId && !isHealWeapon) {
    std::cout << "[PVP DEBUG] cortado: self target sin heal" << std::endl;
    return;
  }

  // Obtenemos al objetivo.
  Player &target = world.getPlayer(targetId);

  std::cout << "[PVP DEBUG] target hp="
            << target.getHp()
            << "/"
            << target.getMaxHp()
            << " isAlive="
            << target.isAlive()
            << " isGhost="
            << target.isGhost()
            << std::endl;

  // Si el objetivo está muerto o fantasma, no puede recibir ataque/curación.
  if (!target.isAlive() || target.isGhost() || target.getHp() == 0) {
    std::cout << "[PVP DEBUG] cortado: target muerto/fantasma/hp0 targetId="
              << targetId
              << std::endl;
    return;
  }

  // Caso flauta/heal.
  if (isHealWeapon) {
    std::cout << "[PVP DEBUG] entra por HEAL" << std::endl;

    if (itemEffectHandler.apply(*weapon, attacker, &target)) {
      std::cout << "[PVP DEBUG] heal aplicado targetHp="
                << target.getHp()
                << " attackerMana="
                << attacker.getMana()
                << std::endl;

      if (attackerId == targetId) {
        sendStats(attackerId, attacker, monitor);
      } else {
        sendStats(attackerId, attacker, monitor);
        sendStats(targetId, target, monitor);
      }

      monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
          targetId,
          static_cast<uint16_t>(target.getHp()),
          static_cast<uint16_t>(target.getMaxHp())));

      monitor.broadcast(std::make_shared<const PlayerAttackVisualMessage>(
          attackerId,
          targetId,
          PlayerAttackVisualType::Heal,
          weapon->stats.visualEffectId));
    } else {
      std::cout << "[PVP DEBUG] heal falló" << std::endl;

      sendCombatChat(attackerId,
                     "No tenés mana suficiente para curar.",
                     ChatMsgType::INFO,
                     monitor);

      sendStats(attackerId, attacker, monitor);
    }

    return;
  }

  // Desde acá es ataque hostil.
  const Tile &attackerTile =
      world.getTileAt(attacker.getTileX(), attacker.getTileY());

  const Tile &targetTile =
      world.getTileAt(target.getTileX(), target.getTileY());


  if (attackerTile.zone == ZoneType::SAFE ||
      targetTile.zone == ZoneType::SAFE) {
    std::cout << "[PVP DEBUG] cortado: zona segura" << std::endl;

    sendCombatChat(attackerId,
                   "No podés atacar jugadores en zona segura.",
                   ChatMsgType::INFO,
                   monitor);
    return;
      }

  std::cout << "[PVP DEBUG] antes de combatSystem.attackPlayer" << std::endl;

  CombatSystem::Result result =
      combatSystem.attackPlayer(attacker, target, world);

  std::cout << "[PVP DEBUG] result valid="
            << result.valid
            << " damage="
            << result.damage
            << " dodged="
            << result.dodged
            << " killed="
            << result.killed
            << " failReason="
            << static_cast<int>(result.failReason)
            << std::endl;

  // Si el ataque no fue válido, avisamos el motivo cuando corresponde
  // y terminamos sin aplicar feedback de daño.
  if (!result.valid) {
    if (result.failReason == CombatSystem::Result::FailReason::FRIENDLY_FIRE) {
      sendCombatChat(attackerId,
                     "No podés atacar a tus aliados.",
                     ChatMsgType::INFO,
                     monitor);
    } else if (result.failReason == CombatSystem::Result::FailReason::NO_MANA) {
      sendCombatChat(attackerId,
                     "No tenés mana suficiente para realizar ese hechizo.",
                     ChatMsgType::INFO,
                     monitor);
    } else if (result.failReason == CombatSystem::Result::FailReason::LEVEL_TOO_LOW) {
      sendCombatChat(attackerId,
                     "No podés atacar a jugadores de nivel bajo.",
                     ChatMsgType::INFO,
                     monitor);
    } else if (result.failReason == CombatSystem::Result::FailReason::LEVEL_DIFF_TOO_HIGH) {
      sendCombatChat(attackerId,
                     "La diferencia de nivel es demasiado grande para atacar.",
                     ChatMsgType::INFO,
                     monitor);
    }

    // Reenviamos stats del atacante por si el intento consumió algo.
    sendStats(attackerId, attacker, monitor);
    return;
  }

  // Si el ataque fue válido y no fue esquivado,
  // avisamos al cliente que debe mostrar el efecto visual.
  if (!result.dodged) {
    monitor.sendTo(attackerId,
                   std::make_shared<const CombatLogMessage>(
                       std::to_string(targetId)));

    broadcastPlayerAttackVisual(attackerId, targetId, attacker, monitor);
  }

  // Si esquivó, no hay daño real.
  // Avisamos a ambos jugadores y terminamos.
  if (result.dodged) {
    sendCombatChat(attackerId,
                   "¡Tu ataque fue esquivado!",
                   ChatMsgType::INFO,
                   monitor);

    sendCombatChat(targetId,
                   "¡Esquivaste el ataque!",
                   ChatMsgType::INFO,
                   monitor);

    sendStats(attackerId, attacker, monitor);
    sendStats(targetId, target, monitor);
    return;
  }

  // Si el golpe mató al jugador, procesamos muerte PvP.
  if (result.killed || target.getHp() <= 0) {
    DeathResult deathResult =
        world.handlePlayerDeath(targetId, attackerId);

    // Si el jugador muerto tenía oro en exceso, lo tiramos al suelo.
    if (deathResult.excessGold > 0) {
      monitor.broadcast(std::make_shared<const GoldOnGroundMessage>(
          deathResult.goldInstanceId,
          deathResult.excessGold,
          deathResult.tileX,
          deathResult.tileY));
    }

    // Avisamos por chat al asesino.
    sendCombatChat(attackerId,
                   "¡Mataste a " + target.getName() + "!",
                   ChatMsgType::DAMAGE_DEALT,
                   monitor);

    // Avisamos por chat al muerto.
    sendCombatChat(targetId,
                   "¡Fuiste asesinado por " + attacker.getName() + "!",
                   ChatMsgType::DAMAGE_TAKEN,
                   monitor);

    // Si la muerte dropeó ítems, los mostramos en el suelo.
    for (const Item &item : deathResult.droppedItems) {
      monitor.broadcast(std::make_shared<const ItemOnGroundMessage>(
          item,
          deathResult.tileX,
          deathResult.tileY));
    }

    // Actualizamos atacante por experiencia/oro/level.
    sendStats(attackerId, attacker, monitor);
    sendLevelUpIfNeeded(attackerId, attacker, monitor);

    // Actualizamos inventario del muerto por drops.
    sendInventory(targetId, target, monitor);

    // Avisamos al jugador muerto que murió.
    // Esto permite que su cliente pase a estado fantasma.
    monitor.sendTo(targetId,
                   std::make_shared<const PlayerDiedMessage>(targetId));

    // Avisamos a todos los demás clientes que ese jugador murió.
    // Esto permite que los remotos lo vean como fantasma.
    monitor.broadcast(std::make_shared<const PlayerDiedMessage>(targetId));

    // Actualizamos el HUD del jugador muerto.
    sendStats(targetId, target, monitor);

    // Avisamos a todos que la vida del target quedó en 0.
    // Esto actualiza la barra de vida remota.
    monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
        targetId,
        static_cast<uint16_t>(target.getHp()),
        static_cast<uint16_t>(target.getMaxHp())));

    return;
  }

  // Caso normal: golpe válido, no esquivado, no mató.
  // Avisamos al atacante cuánto daño causó.
  sendCombatChat(attackerId,
                 "Causaste " + std::to_string(result.damage) +
                     " pts de daño a " + target.getName() + ".",
                 ChatMsgType::DAMAGE_DEALT,
                 monitor);

  // Avisamos al jugador atacado cuánto daño recibió.
  sendCombatChat(targetId,
                 attacker.getName() + " te causó " +
                     std::to_string(result.damage) + " pts de daño.",
                 ChatMsgType::DAMAGE_TAKEN,
                 monitor);

  // Damos experiencia si corresponde.
  world.giveExperience(attackerId, result.expGained);

  // Actualizamos HUD de ambos jugadores.
  sendStats(attackerId, attacker, monitor);
  sendStats(targetId, target, monitor);

  // Si subió de nivel, avisamos.
  sendLevelUpIfNeeded(attackerId, attacker, monitor);

  // Actualizamos la barra de vida remota sobre el jugador atacado.
  monitor.broadcast(std::make_shared<const PlayerHealthMessage>(
      targetId,
      static_cast<uint16_t>(target.getHp()),
      static_cast<uint16_t>(target.getMaxHp())));
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